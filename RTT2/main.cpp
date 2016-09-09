#include <iostream>
#include <sstream>
#include <cfenv>

#include "vec.h"
#include "mat.h"
#include "rasterizer.h"
#include "window.h"
#include "buffer.h"
#include "model.h"
#include "renderer.h"

using namespace rtt2;

#define WND_WIDTH 1024
#define WND_HEIGHT 768

#define SHADOW_BUFFER_SIZE 5000

//#define TEAPOT

#ifdef TEAPOT
#	define MOVE_MULT 50
#	define ROT_DIV 2000.0
#	define INIT_CAM_X -200.0
#	define ZNEAR 30.0
#	define ZFAR 500.0
#else
#	define MOVE_MULT 0.5
#	define ROT_DIV 2000.0
#	define INIT_CAM_X -5.0
#	define ZNEAR 0.8
#	define ZFAR 50.0
#endif

LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);

window wnd("rtt2", wndproc);
bool goon = true, iscam = false;
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

int main() {
	sys_color_buffer scb(wnd.get_dc(), WND_WIDTH, WND_HEIGHT);
	mem_depth_buffer mdb(WND_WIDTH, WND_HEIGHT);
	rasterizer rast;
	fps_counter counter;
	stopwatch stw;
	basic_renderer rend;
	camera_spec cam;
	mat4 cammod, camproj;
	scene_cache defsc;
	buffer_set defbsc;

	texture tex;
	model_data mdl1, mdl2;
	mat4 mm1, mm2;
	material_phong mtrl(1.0, 1.0, 50.0);

	//directional_light l1;
	spotlight l2, l3;

	mem_depth_buffer s2ds(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE), s3ds(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE);
	buffer_set s2bs, s3bs;

	defbsc.set(WND_WIDTH, WND_HEIGHT, scb.get_arr(), mdb.get_arr(), nullptr);

	rast.cur_buf = defbsc;
	rast.shader_vtx = rasterizer::default_vertex_shader;
	rast.shader_test = rasterizer::default_test_shader;
	rast.shader_frag = rasterizer::default_fragment_shader_notex;

	counter.set_window(0.5);

	wnd.set_client_size(WND_WIDTH, WND_HEIGHT);
	wnd.set_center();
	wnd.show();

	cam.hori_fov = 60.0 * RTT2_PI / 180.0;
	cam.aspect_ratio = WND_HEIGHT / static_cast<rtt2_float>(WND_WIDTH);
	cam.znear = ZNEAR;
	cam.zfar = ZFAR;
	cam.pos = vec3(INIT_CAM_X, 0.0, 0.0);
	cam.forward = vec3(1.0, 0.0, 0.0);
	cam.up = vec3(0.0, 0.0, 1.0);
	cam.make_cache();

	{
		file_access in("rsrc/tex.ppm", "r");
		tex.load_ppm(in);
	}
	tex.make_float_cache_nocheck();

	{
		file_access in("rsrc/buddha.obj", "r");
		mdl1.load_obj(in);
	}
	if (mdl1.normals.size() == 1) {
		mdl1.generate_normals_weighted_average();
	}

	mdl2.points.push_back(vec3(-20.0, -20.0, 0.0));
	mdl2.points.push_back(vec3(20.0, -20.0, 0.0));
	mdl2.points.push_back(vec3(-20.0, 20.0, 0.0));
	mdl2.points.push_back(vec3(20.0, 20.0, 0.0));
	mdl2.uvs.push_back(vec2(-10.0, -10.0));
	mdl2.uvs.push_back(vec2(10.0, -10.0));
	mdl2.uvs.push_back(vec2(-10.0, 10.0));
	mdl2.uvs.push_back(vec2(10.0, 10.0));
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

	l2.color = color_vec_rgb(9000.0, 3000.0, 2000.0);
	l2.pos = vec3(5.0, 60.0, 95.0);
	l2.dir = vec3(-5.0, -60.0, -95.0);
	l2.dir.set_length(1.0);
	l2.inner_angle = 0.06;
	l2.outer_angle = 0.07;

	l3.color = color_vec_rgb(2000.0, 3000.0, 9000.0);
	l3.pos = vec3(35.0, -50.0, 95.0);
	l3.dir = vec3(-35.0, 50.0, -95.0);
	l3.dir.set_length(1.0);
	l3.inner_angle = 0.07;
	l3.outer_angle = 0.08;

	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.linked_rasterizer = &rast;
	rend.models.push_back(model(&mdl1, &mtrl, &mm1, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));
	rend.models.push_back(model(&mdl2, &mtrl, &mm2, &tex, color_vec(1.0, 1.0, 1.0, 1.0)));
	//rend.lights.push_back(light(l1));
	rend.lights.push_back(light(l2));
	rend.lights.push_back(light(l3));
	rend.cache = &defsc;
	rend.init_cache();
	rend.refresh_cache();

	get_trans_camview_3(cam, cammod);
	get_trans_frustrum_3(cam, camproj);

	shadow_settings set;
	set.of_spotlight.tolerance = 0.001;
	set.of_spotlight.znear = 100.0;
	set.of_spotlight.zfar = 150.0;
	s2bs.set(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s2ds.get_arr(), nullptr);
	rend.lights[0].build_shadow_cache(rend, s2bs, set);
	s3bs.set(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s3ds.get_arr(), nullptr);
	rend.lights[1].build_shadow_cache(rend, s3bs, set);

	rast.cur_buf = defbsc;
	rend.linked_rasterizer = &rast;
	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.cache = &defsc;

	while (goon) {
		rtt2_float delta = stw.tick_in_seconds();

		while (wnd.idle()) {
		}
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);

		if (iscam) {
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

			mat4 invmod;
			cammod.get_inversion(invmod);
			for (auto i = rend.lights.begin(); i != rend.lights.end(); ++i) {
				if (i->has_shadow) {
					i->set_inverse_camera_mat(invmod);
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

			if (!is_key_down(VK_RBUTTON)) {
				iscam = false;
			}
		}

		rend.setup_rendering_env();
		rend.render_cached();

		// end of tests
		scb.display(wnd.get_dc());

		counter.update();
		std::stringstream ss;
		ss << "rasterizer test | fps: " << counter.get_fps();
		wnd.set_caption(ss.str());
	}
	//std::system("pause");
	return 0;
}
