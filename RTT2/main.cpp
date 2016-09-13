#include <iostream>
#include <sstream>
#include <cfenv>
#include <thread>

#include "vec.h"
#include "mat.h"
#include "rasterizer.h"
#include "window.h"
#include "buffer.h"
#include "model.h"
#include "renderer.h"
#include "enhancement.h"

using namespace rtt2;

#define WND_WIDTH 1200
#define WND_HEIGHT 900

#define BUF_WIDTH 400
#define BUF_HEIGHT 300

#define SHADOW_BUFFER_SIZE 5000

#define USE_MODEL
#define MODEL_FILE "rsrc/buddha.obj"

#define MOVE_MULT 0.5
#define ROT_DIV 2000.0
#define INIT_CAM_POS -5.0, -4.0, 3.0
#define ZNEAR 0.8
#define ZFAR 50.0

LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);

window wnd("rtt2", wndproc);
bool goon = true, iscam = false, licam = false;
vec2 mouse(0, 0);

rtt2_float ca_hor = 0.0, ca_vert = 0.0;

LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_CLOSE:
			wnd.hide();
			goon = false;
			return 0;
		case WM_MOUSEMOVE:
			SHORT x, y;
			get_xy_from_lparam(lparam, x, y);
			mouse.x = x - 0.5;
			mouse.y = WND_HEIGHT - y - 0.5;
			return 0;
		case WM_KEYDOWN:
			return 0;
		case WM_RBUTTONDOWN:
			iscam = true;
			wnd.put_mouse_to_center();
			return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

basic_renderer rend;
mem_color_buffer full_rendering_buf(WND_WIDTH, WND_HEIGHT);
sys_color_buffer finalbuf(wnd.get_dc(), WND_WIDTH, WND_HEIGHT);
task_lock render_task;

void render_full_fx_impl() {
	if (render_task.try_start()) {
		std::cout << "rendering full fx...";
		rasterizer rast;
		mem_depth_buffer mdb(WND_WIDTH, WND_HEIGHT);
		rast.cur_buf.set(WND_WIDTH, WND_HEIGHT, full_rendering_buf.get_arr(), mdb.get_arr(), nullptr);
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);
		rend.linked_rasterizer = &rast;
		rend.setup_rendering_env();
		long long t1, t2;
		t1 = get_time();
		rend.render_cached();
		t2 = get_time();
		if (render_task.get_status() != task_status::cancelled) {
			enlarged_copy(full_rendering_buf, finalbuf);
			finalbuf.display(wnd.get_dc());
			std::cout << " done with " << (static_cast<rtt2_float>(get_timer_freq()) / (t2 - t1)) << " fps\n";
		} else {
			std::cout << " cancelled\n";
		}
		render_task.on_stopped();
	}
}
void render_full_fx() {
	std::thread renderer_thread(render_full_fx_impl);
	renderer_thread.detach();
}
void cancel_render_full_fx() {
	if (render_task.cancel()) {
		rend.as_task.cancel_and_wait();
		render_task.wait();
	}
}

scene_description scene;
mem_depth_buffer s2ds(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE), s3ds(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE);
mat4 inv_cam;

void render_shadows_impl() {
	std::cout << "generating shadow map...";
	shadow_settings set;
	set.of_spotlight.tolerance = 0.001;
	set.of_spotlight.znear = 100.0;
	set.of_spotlight.zfar = 150.0;
	scene.lights[0].build_shadow_cache(scene, buffer_set(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s2ds.get_arr(), nullptr), set);
	scene.lights[1].build_shadow_cache(scene, buffer_set(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s3ds.get_arr(), nullptr), set);
	for (auto i = scene.lights.begin(); i != scene.lights.end(); ++i) {
		if (i->has_shadow) {
			i->set_inverse_camera_mat(inv_cam);
		}
	}
	std::cout << " done\n";
}
void stop_render_shadows() {
}

int main() {
	mem_color_buffer screen_buf(BUF_WIDTH, BUF_HEIGHT);
	mem_depth_buffer mdb(BUF_WIDTH, BUF_HEIGHT);
	rasterizer rast;
	stopwatch stw;
	camera_spec cam;
	mat4 cammod, camproj;
	scene_cache defsc;

	texture tex, dep;
	model_data mdl1, mdl2;
	mat4 mm1, mm2;
	material_phong mtrl(1.0, 1.0, 50.0);
	parallax_occulusion_mapping_enhancement test_po;

	//directional_light l1;
	spotlight l2, l3;

	task_toggler full_render(render_full_fx, cancel_render_full_fx), draw_shadows(render_shadows_impl, stop_render_shadows);
	bool lastf1down = false, lastf2down = false;

	draw_shadows.affected = &full_render;

	rast.cur_buf.set(BUF_WIDTH, BUF_HEIGHT, screen_buf.get_arr(), mdb.get_arr(), nullptr);

	cam.hori_fov = 60.0 * RTT2_PI / 180.0;
	cam.aspect_ratio = BUF_HEIGHT / static_cast<rtt2_float>(BUF_WIDTH);
	cam.znear = ZNEAR;
	cam.zfar = ZFAR;
	cam.pos = vec3(INIT_CAM_POS);
	cam.forward = vec3(1.0, 0.0, 0.0);
	cam.up = vec3(0.0, 0.0, 1.0);
	cam.make_cache();

	std::cout << "loading textures...";
	{
		file_access in("rsrc/po_img.ppm", "r");
		tex.load_ppm(in);
	}
	tex.make_float_cache_nocheck();

	{
		file_access in("rsrc/po_depth.ppm", "r");
		dep.load_ppm(in);
	}
	dep.make_float_cache_nocheck();
	std::cout << " done\n";

#ifdef USE_MODEL
	std::cout << "loading models...";
	{
		file_access in(MODEL_FILE, "r");
		mdl1.load_obj(in);
	}
	if (mdl1.normals.size() == 1) {
		mdl1.generate_normals_weighted_average();
	}
	std::cout << " done\n";
#endif

	test_po.depth = 0.2;
	test_po.depth_tex = &dep;

	mdl2.points.push_back(vec3(-20.0, -20.0, 0.0));
	mdl2.points.push_back(vec3(20.0, -20.0, 0.0));
	mdl2.points.push_back(vec3(-20.0, 20.0, 0.0));
	mdl2.points.push_back(vec3(20.0, 20.0, 0.0));
	mdl2.uvs.push_back(vec2(5.0, 5.0));
	mdl2.uvs.push_back(vec2(-5.0, 5.0));
	mdl2.uvs.push_back(vec2(5.0, -5.0));
	mdl2.uvs.push_back(vec2(-5.0, -5.0));
	mdl2.normals.push_back(vec3(0.0, 0.0, 1.0));
	model_data::face_info fi;
	fi.uv_ids[0] = 0;
	fi.uv_ids[1] = 1;
	fi.uv_ids[2] = 2;
	fi.vertex_ids[0] = 0;
	fi.vertex_ids[1] = 1;
	fi.vertex_ids[2] = 2;
	fi.normal_ids[0] = 0;
	fi.normal_ids[1] = 0;
	fi.normal_ids[2] = 0;
	mdl2.faces.push_back(fi);
	fi.uv_ids[0] = 2;
	fi.uv_ids[2] = 3;
	fi.vertex_ids[0] = 2;
	fi.vertex_ids[2] = 3;
	mdl2.faces.push_back(fi);
	mm2.set_identity();

	get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), RTT2_PI * 0.5, mm1);

	/*l1.color = color_vec_rgb(1.0, 0.3, 0.4);
	l1.dir = vec3(-3.0, -4.0, -5.0);
	l1.dir.set_length(1.0);*/

	std::cout << "setting up lights...";
	l2.color = color_vec_rgb(9000.0, 6000.0, 5000.0);
	l2.pos = vec3(5.0, 95.0, 60.0);
	l2.dir = vec3(-5.0, -95.0, -60.0);
	l2.dir.set_length(1.0);
	l2.inner_angle = 0.06;
	l2.outer_angle = 0.07;

	l3.color = color_vec_rgb(5000.0, 6000.0, 9000.0);
	l3.pos = vec3(60.0, -5.0, 95.0);
	l3.dir = vec3(-60.0, 5.0, -95.0);
	l3.dir.set_length(1.0);
	l3.inner_angle = 0.07;
	l3.outer_angle = 0.08;
	std::cout << " done\n";

#ifdef USE_MODEL
	scene.models.push_back(model(&mdl1, &mtrl, &mm1, nullptr, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));
#endif
	scene.models.push_back(model(&mdl2, &mtrl, &mm2, &tex, &test_po, color_vec(1.0, 1.0, 1.0, 1.0)));
	//scene.lights.push_back(light(l1));
	scene.lights.push_back(light(l2));
	scene.lights.push_back(light(l3));

	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.linked_rasterizer = &rast;
	rend.scene = &scene;
	rend.cache = &defsc;
	rend.init_cache();
	rend.refresh_cache();

	get_trans_camview_3(cam, cammod);
	get_trans_frustrum_3(cam, camproj);

	rend.linked_rasterizer = &rast;
	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.cache = &defsc;

	wnd.set_client_size(WND_WIDTH, WND_HEIGHT);
	wnd.set_center();
	wnd.show();

	while (goon) {
		rtt2_float delta = stw.tick_in_seconds();

		while (wnd.idle()) {
		}
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);

		bool f1d = is_key_down(VK_F1);
		if (f1d != lastf1down) {
			if (f1d) {
				draw_shadows.add_barrier();
			} else {
				draw_shadows.remove_barrier();
			}
		}
		lastf1down = f1d;
		if (f1d) {
			test_po.depth -= 0.1 * delta;
		}

		bool f2d = is_key_down(VK_F2);
		if (f2d != lastf2down) {
			if (f2d) {
				draw_shadows.add_barrier();
			} else {
				draw_shadows.remove_barrier();
			}
		}
		lastf2down = f2d;
		if (f2d) {
			test_po.depth += 0.1 * delta;
		}

		if (licam != iscam) {
			if (iscam) {
				full_render.add_barrier();
			} else {
				full_render.remove_barrier();
			}
		}
		licam = iscam;
		if (iscam) {
			rend.linked_rasterizer = &rast;

			int x, y, cx, cy;
			get_mouse_global_pos(x, y);
			wnd.get_center_pos(cx, cy);
			ca_hor += (cx - x) / ROT_DIV;
			ca_vert += (cy - y) / ROT_DIV;
			wnd.put_mouse_to_center();
			rtt2_float coshv = std::cos(ca_vert);
			cam.forward = vec3(std::cos(ca_hor) * coshv, std::sin(ca_hor) * coshv, std::sin(ca_vert));
			cam.up = vec3::cross(vec3(cam.forward.y, -cam.forward.x, 0.0), cam.forward);
			cam.up.set_length(1.0);
			cam.make_cache();
			get_trans_camview_3(cam, cammod);
			get_trans_frustrum_3(cam, camproj);

			cammod.get_inversion(inv_cam);
			for (auto i = scene.lights.begin(); i != scene.lights.end(); ++i) {
				if (i->has_shadow) {
					i->set_inverse_camera_mat(inv_cam);
				}
			}

			rtt2_float mult = delta * MOVE_MULT;
			if (is_key_down(VK_SHIFT)) {
				mult *= 5;
			}
			if (is_key_down('W')) {
				cam.pos += cam.forward * mult;
			}
			if (is_key_down('S')) {
				cam.pos -= cam.forward * mult;
			}
			if (is_key_down('A')) {
				cam.pos -= cam.right_cache * mult;
			}
			if (is_key_down('D')) {
				cam.pos += cam.right_cache * mult;
			}

			rend.refresh_cache();

			rend.setup_rendering_compact_env();
			rend.render_cached();

			enlarged_copy(screen_buf, finalbuf);
			finalbuf.display(wnd.get_dc());

			if (!is_key_down(VK_RBUTTON)) {
				iscam = false;
			}
		}
	}
	cancel_render_full_fx();
	return 0;
}
