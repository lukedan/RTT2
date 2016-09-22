#include <iostream>
#include <sstream>
#include <cfenv>
#include <thread>
#include <random>

#include "vec.h"
#include "mat.h"
#include "rasterizer.h"
#include "window.h"
#include "buffer.h"
#include "model.h"
#include "renderer.h"
#include "enhancement.h"

using namespace rtt2;

#define WND_WIDTH 960
#define WND_HEIGHT 540

#define BUF_WIDTH 192
#define BUF_HEIGHT 108

#define SHADOW_BUFFER_SIZE 1000

#define USE_MODEL
#define MODEL_NUM 20
//#define USE_GROUND

#define MODEL_FILE "rsrc/cube.obj"

#define MOVE_MULT 0.5
#define ROT_DIV 500.0
#define INIT_CAM_POS -5.0, -4.0, 3.0
#define ZNEAR 0.8
#define ZFAR 100.0

LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);

window wnd("rtt2", wndproc);
bool goon = true, iscam = false, licam = false;
vec2 mouse(0, 0);

rtt2_float ca_hor = 0.0, ca_vert = 0.0;
camera_spec cam;

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
		case WM_RBUTTONDOWN:
			iscam = true;
			wnd.put_mouse_to_center();
			return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

mem_color_buffer full_rendering_buf(WND_WIDTH, WND_HEIGHT);
sys_color_buffer finalbuf(wnd.get_dc(), WND_WIDTH, WND_HEIGHT);
mat4 cammod, camproj;
task_lock render_task;
void render_full_fx_impl();
void render_full_fx() {
	std::thread renderer_thread(render_full_fx_impl);
	renderer_thread.detach();
}
void cancel_render_full_fx() {
	render_task.cancel_and_wait();
}
task_toggler full_render(render_full_fx, cancel_render_full_fx);

scene_cache defsc;
scene_description scene;
mem_depth_buffer s2ds(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE), s3ds[6]{
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE }
};
task_lock gen_shadows;
void render_shadows_impl();
void render_shadows() {
	std::thread renderer_thread(render_shadows_impl);
	renderer_thread.detach();
	full_render.add_barrier();
}
void cancel_rendering_shadows() {
	gen_shadows.cancel_and_wait();
}
task_toggler draw_shadows(render_shadows, cancel_rendering_shadows);

void render_volumetric_shadow(const scene_description &sd, scene_cache &sc, const mat4 &proj, const mem_depth_buffer &db, const camera_spec &cs, const buffer_set &bs, size_t split = 100) {
	rtt2_float zinc = (cs.zfar - cs.znear) / split, xsc = std::tan(cs.hori_fov * 0.5), ysc = xsc * cs.aspect_ratio;
	for (size_t y = 0; y < bs.h; ++y) {
		for (size_t x = 0; x < bs.w; ++x) {
			rtt2_float cur = -cs.znear, d = *db.get_at(x, y);
			color_vec_rgb res(0.0, 0.0, 0.0);
			for (size_t i = 0; i < split; ++i, cur -= zinc) {
				rtt2_float crv = cur - zinc * std::rand() / static_cast<rtt2_float>(RAND_MAX);
				rtt2_float rx = (2 * x + 1) / static_cast<rtt2_float>(bs.w) - 1.0, ry = (2 * y + 1) / static_cast<rtt2_float>(bs.h) - 1.0;
				rx *= -crv * xsc;
				ry *= -crv * ysc;
				vec3 rp(rx, ry, crv);
				vec4 rs = proj * vec4(rp);
				rtt2_float cz = rs.z / rs.w;
				if (cz < d) {
					break;
				}
				for (size_t cl = 0; cl < sd.lights.size(); ++cl) {
					vec3 ti;
					color_vec_rgb c;
					if (sd.lights[cl].data->get_illum(sc.of_lights[cl], rp, ti, c)) {
						if (!sd.lights[cl].in_shadow(rp)) {
							res += c * 0.1; // TODO refine the formula
						}
					}
				}
			}
			res *= zinc;
			clamp_vec(res, 0.0, 1.0);
			bs.get_at(x, y, bs.color_arr)->from_vec4(vec4(res, 1.0));
		}
	}
}

#define SH_WIDTH (1920 * 4)
#define SH_HEIGHT (1080 * 4)
void render_superhigh() {
	std::cout << "rendering SUPERHIGH...";
	basic_renderer rend;
	rasterizer rast;
	mem_depth_buffer mdb(SH_WIDTH, SH_HEIGHT);
	mem_color_buffer mcb(SH_WIDTH, SH_HEIGHT), shbuf(SH_WIDTH, SH_HEIGHT);
	rast.cur_buf.set(SH_WIDTH, SH_HEIGHT, shbuf.get_arr(), mdb.get_arr(), nullptr);
	rast.clear_color_buf(device_color(255, 0, 0, 0));
	rast.clear_depth_buf(-1.0);
	mcb.clear(device_color(0, 0, 0, 0));
	rend.scene = &scene;
	rend.cache = &defsc;
	rend.linked_rasterizer = &rast;
	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.setup_rendering_env();
	long long t1, t2;
	t1 = get_time();
	rend.render_cached();
	render_volumetric_shadow(scene, defsc, camproj, mdb, cam, buffer_set(SH_WIDTH, SH_HEIGHT, mcb.get_arr(), nullptr, nullptr), 100);
	device_color *a = mcb.get_arr(), *b = shbuf.get_arr();
	for (size_t i = SH_WIDTH * SH_HEIGHT; i > 0; --i, ++a, ++b) {
		vec4 av, bv;
		a->to_vec4(av);
		b->to_vec4(bv);
		av += bv;
		clamp_vec(av, 0.0, 1.0);
		b->from_vec4(av);
	}
	texture t;
	t.from_buffer_nocheck(shbuf);
	{
		file_access out("sh.ppm", "w");
		t.save_ppm(out);
	}
	t2 = get_time();
	std::cout << "  done with " << (static_cast<rtt2_float>(get_timer_freq()) / (t2 - t1)) << " fps\n";
}
void render_full_fx_impl() {
	if (render_task.try_start()) {
		std::cout << "rendering full fx...";
		basic_renderer rend;
		rasterizer rast;
		mem_depth_buffer mdb(WND_WIDTH, WND_HEIGHT);
		mem_color_buffer mcb(WND_WIDTH, WND_HEIGHT);
		rast.cur_buf.set(WND_WIDTH, WND_HEIGHT, full_rendering_buf.get_arr(), mdb.get_arr(), nullptr);
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);
		mcb.clear(device_color(0, 0, 0, 0));
		rend.scene = &scene;
		rend.cache = &defsc;
		rend.linked_rasterizer = &rast;
		rend.mat_modelview = &cammod;
		rend.mat_projection = &camproj;
		rend.setup_rendering_env();
		long long t1, t2;
		t1 = get_time();
		rend.render_cached_async(render_task);
		render_volumetric_shadow(scene, defsc, camproj, mdb, cam, buffer_set(WND_WIDTH, WND_HEIGHT, mcb.get_arr(), nullptr, nullptr), 20);
		device_color *a = mcb.get_arr(), *b = full_rendering_buf.get_arr();
		for (size_t i = WND_WIDTH * WND_HEIGHT; i > 0; --i, ++a, ++b) {
			vec4 av, bv;
			a->to_vec4(av);
			b->to_vec4(bv);
			av += bv;
			clamp_vec(av, 0.0, 1.0);
			b->from_vec4(av);
		}
		t2 = get_time();
		if (render_task.get_status() != task_status::cancelled) {
			enlarged_copy(full_rendering_buf, finalbuf);
			finalbuf.display(wnd.get_dc());
			std::cout << "  done with " << (static_cast<rtt2_float>(get_timer_freq()) / (t2 - t1)) << " fps\n";
		} else {
			std::cout << "  cancelled\n";
		}
		render_task.on_stopped();
	}
}

void render_shadows_impl() {
	if (gen_shadows.try_start()) {
		std::cout << "generating shadow map...";
		shadow_settings set;

		//set.of_spotlight.znear = 2.0;
		//set.of_spotlight.zfar = 20.0;
		//set.of_spotlight.tolerance = 0.001;
		//buffer_set bs(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s2ds.get_arr(), nullptr);
		//scene.lights[0].build_shadow_cache_async(scene, &bs, set, gen_shadows);

		set.of_pointlight.tolerance = 0.001;
		set.of_pointlight.znear = 0.1;
		set.of_pointlight.zfar = 20.0;
		buffer_set bss[6];
		for (size_t i = 0; i < 6; ++i) {
			bss[i].set(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s3ds[i].get_arr(), nullptr);
		}
		scene.lights[0].build_shadow_cache_async(scene, bss, set, gen_shadows);

		for (auto i = scene.lights.begin(); i != scene.lights.end(); ++i) {
			if (i->has_shadow) {
				i->set_camera_mat(cammod);
			}
		}
		if (gen_shadows.get_status() != task_status::cancelled) {
			std::cout << "  done\n";
		} else {
			std::cout << "  cancelled\n";
		}
		gen_shadows.on_stopped();
		full_render.remove_barrier();
	}
}

void shadow_add_barrier() {
	draw_shadows.add_barrier();
}
void shadow_remove_barrier() {
	draw_shadows.remove_barrier();
}
void full_fx_add_barrier() {
	full_render.add_barrier();
}
void full_fx_remove_barrier() {
	full_render.remove_barrier();
}

void set_random_trans_and_rot_matrix(mat4 &mt, rtt2_float minr, rtt2_float maxr) {
	mat4 rot1, rot2, trans;
	vec3 rnd;
	rtt2_float ang1 = get_random_num_01() * RTT2_PI, ang2 = get_random_num_01() * 2.0 * RTT2_PI;
	rnd.set_random_on_unit_sphere(get_random_num_01);
	get_trans_rotation_3(vec3(0.0, 0.0, 0.0), rnd, get_random_num_01() * 2.0 * RTT2_PI, rot1);
	rnd.set_random_on_unit_sphere(get_random_num_01);
	get_trans_translation_3(rnd * (minr + (maxr - minr) * get_random_num_01()), trans);
	mat4::mult_ref(trans, rot1, mt);

	//get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), ang1, rot1);
	//get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), ang2, rot2);
	//rnd = vec3(std::sin(ang1) * std::cos(ang2), std::sin(ang1) * std::sin(ang2), std::cos(ang1));
	//mat4::mult_ref(rot2, rot1, trans);
	//rot1 = trans;
	//get_trans_translation_3(rnd * (minr + (maxr - minr) * get_random_num_01()), trans);
	//mat4::mult_ref(trans, rot1, mt);
}

int main() {
	mem_color_buffer screen_buf(BUF_WIDTH, BUF_HEIGHT);
	mem_depth_buffer mdb(BUF_WIDTH, BUF_HEIGHT);
	rasterizer rast;
	stopwatch stw;
	basic_renderer rend;

	texture tex, dep;
#ifdef USE_MODEL
	model_data mdl1;
	mat4 mm1;
#endif
#ifdef USE_GROUND
	model_data mdl2;
	mat4 mm2;
	parallax_occulusion_mapping_enhancement test_po;
#endif
	material_phong mtrl(1.0, 1.0, 50.0);

	//directional_light l1;
	spotlight l2;
	pointlight l3;

	key_monitor incd, decd;

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

#ifdef USE_GROUND
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
#endif

	get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), RTT2_PI * 0.5, mm1);

	/*l1.color = color_vec_rgb(1.0, 0.3, 0.4);
	l1.dir = vec3(-3.0, -4.0, -5.0);
	l1.dir.set_length(1.0);*/

	std::cout << "setting up lights...";
	//l2.color = color_vec_rgb(90.0, 60.0, 50.0);
	//l2.pos = vec3(0.0, 9.0, 6.0);
	//l2.dir = vec3(0.0, -1.0, -0.5);
	//l2.dir.set_length(1.0);
	//l2.inner_angle = 0.2;
	//l2.outer_angle = 0.4;

	l3.color = color_vec_rgb(1.0, 2.0, 5.0);
	//l3.pos = vec3(0.0, 4.0, 2.0);
#if MODEL_NUM > 1
	l3.pos = vec3(0.0, 0.0, 0.0);
#else
	l3.pos = vec3(5.0, 0.0, 6.0);
#endif
	std::cout << " done\n";

#ifdef USE_GROUND
	scene.models.push_back(model(&mdl2, &mtrl, &mm2, &tex, &test_po, color_vec(1.0, 1.0, 1.0, 1.0)));
#endif
#ifdef USE_MODEL
#	if MODEL_NUM > 1
	std::vector<mat4> ms(MODEL_NUM);
	for (size_t i = 0; i < MODEL_NUM; ++i) {
		set_random_trans_and_rot_matrix(ms[i], 2.0, 5.0);
		scene.models.push_back(model(&mdl1, &mtrl, &ms[i], nullptr, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));
	}
#	else
	scene.models.push_back(model(&mdl1, &mtrl, &mm1, nullptr, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));
#	endif
#endif
	//scene.lights.push_back(light(l1));
	//scene.lights.push_back(light(l2));
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

	incd.set(VK_F1, shadow_add_barrier, shadow_remove_barrier);
	decd.set(VK_F2, shadow_add_barrier, shadow_remove_barrier);

	wnd.set_client_size(WND_WIDTH, WND_HEIGHT);
	wnd.set_center();
	wnd.show();

	while (goon) {
		rtt2_float delta = stw.tick_in_seconds();

		while (wnd.idle()) {
		}
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);

		incd.update();
		decd.update();

		if (licam != iscam) {
			if (iscam) {
				full_render.add_barrier();
			} else {
				full_render.remove_barrier();
			}
		}
		licam = iscam;

		draw_shadows.check();
		full_render.check();

#ifdef USE_GROUND
		if (incd.down() != decd.down()) {
			if (incd.down()) {
				test_po.depth += 0.1 * delta;
				std::cout << "depth = " << test_po.depth << "\n";
			} else {
				test_po.depth -= 0.1 * delta;
				std::cout << "depth = " << test_po.depth << "\n";
			}
		}
#endif
		if (is_key_down('Q')) {
			render_superhigh();
		}
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

			for (auto i = scene.lights.begin(); i != scene.lights.end(); ++i) {
				if (i->has_shadow) {
					i->set_camera_mat(cammod);
				}
			}

			rtt2_float mult = delta * MOVE_MULT;
			if (is_key_down(VK_SHIFT)) {
				mult *= 20;
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

			rend.setup_compact_rendering_env();
			rend.render_cached();

			enlarged_copy(screen_buf, finalbuf);
			finalbuf.display(wnd.get_dc());

			if (!is_key_down(VK_RBUTTON)) {
				iscam = false;
			}
		}
	}
	cancel_rendering_shadows();
	cancel_render_full_fx();
	return 0;
}
