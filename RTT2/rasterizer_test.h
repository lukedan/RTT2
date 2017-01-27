#include <iostream>
#include <sstream>
#include <random>

#include "vec.h"
#include "mat.h"
#include "rasterizer.h"
#include "window.h"
#include "buffer.h"
#include "model.h"
#include "renderer.h"
#include "enhancement.h"
#include "raytracer.h"

using namespace rtt2;

#define WND_WIDTH 1440
#define WND_HEIGHT 900

#define SHADOW_BUFFER_SIZE 1000
#define MODEL_FILE "rsrc/buddha.obj"
#define TEXTURE_FILE "rsrc/po_img.ppm"
#define BUF_WIDTH 720
#define BUF_HEIGHT 450

#define ROT_DIV 500.0
#define ZNEAR 0.8
#define INIT_CAM_POS -5.0, -4.0, 3.0
#define MOVE_MULT 0.5
#define ZFAR 100.0

LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);

window wnd("rtt2", wndproc);
bool goon = true, iscam = false, licam = false;
vec2 mouse(0, 0);

rtt2_float ca_hor = 1.5 * RTT2_PI, ca_vert = 0.0;
camera cam;

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

std::default_random_engine eng;
std::uniform_real_distribution<rtt2_float> dist(0.0, 1.0);

mem_color_buffer full_rendering_buf(WND_WIDTH, WND_HEIGHT);
sys_color_buffer finalbuf(wnd.get_dc(), WND_WIDTH, WND_HEIGHT);
rasterizing::rasterizer rast;
rasterizing::basic_renderer rend;
mat4 cammod, camproj;

texture tex, dep;
rasterizing::scene_cache defsc;
rasterizing::scene_description scene;
brdf_phong mtrl(1.0, 1.0, 50.0);
model_data mdl1;
mat4 mm1;
model_data mdl2;
mat4 mm2;
rasterizing::parallax_occulusion_mapping_enhancement test_po;
rasterizing::spot_light_data l2;
rasterizing::point_light_data l3;

void initialize_scene() {
	std::cout << "loading textures...";
	{
		std::ifstream in(TEXTURE_FILE);
		tex.load_ppm(in);
	}
	tex.make_float_cache_nocheck();

	{
		std::ifstream in("rsrc/po_depth.ppm");
		dep.load_ppm(in);
	}
	dep.make_float_cache_nocheck();
	std::cout << " done\n";

	std::cout << "loading models...";
	{
		std::ifstream in(MODEL_FILE);
		mdl1.load_obj(in);
	}
	if (mdl1.normals.size() == 1) {
		mdl1.generate_normals_weighted_average();
	}
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
	std::cout << " done\n";

	get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), RTT2_PI * 0.5, mm1);

	std::cout << "setting up lights...";
	l2.color = color_vec_rgb(30.0, 20.0, 10.0);
	l2.pos = vec3(0.0, -4.0, 15.0);
	l2.dir = vec3(0.0, 0.5, -1.5);
	l2.dir.set_length(1.0);
	l2.inner_angle = 0.2;
	l2.outer_angle = 0.4;

	l3.pos = vec3(5.0, 0.0, 6.0);
	l3.color = color_vec_rgb(3.0, 10.0, 20.0);

	scene.lights.push_back(rasterizing::light(l2));
	scene.lights.push_back(rasterizing::light(l3));

	std::cout << " done\n";

	scene.models.push_back(rasterizing::model(&mdl1, &mtrl, &mm1, &tex, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));
	test_po.depth = 0.2;
	test_po.depth_tex = &dep;
	scene.models.push_back(rasterizing::model(&mdl2, &mtrl, &mm2, &tex, &test_po, color_vec(1.0, 1.0, 1.0, 1.0)));

	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.linked_rasterizer = &rast;
	rend.scene = &scene;
	rend.cache = &defsc;
	rend.init_cache();
	rend.refresh_cache();
}

rasterizing::mem_depth_buffer s2ds(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE), s3ds[6]{
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE },
	{ SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE }
};

void render_full_fx();
void render_shadows();

void render_volumetric_shadow(const rasterizing::scene_description &sd, rasterizing::scene_cache &sc, const mat4 &proj, const rasterizing::mem_depth_buffer &db, const camera &cs, const rasterizing::buffer_set &bs, size_t split = 100) {
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
void render_shadows() {
	std::cout << "generating shadow map...";
	rasterizing::shadow_settings set;

	set.of_spotlight.znear = 2.0;
	set.of_spotlight.zfar = 60.0;
	set.of_spotlight.tolerance = 0.001;
	rasterizing::buffer_set bs(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s2ds.get_arr(), nullptr);
	scene.lights[0].build_shadow_cache(scene, &bs, set);

	set.of_pointlight.tolerance = 0.001;
	set.of_pointlight.znear = 0.1;
	set.of_pointlight.zfar = 60.0;
	rasterizing::buffer_set bss[6];
	for (size_t i = 0; i < 6; ++i) {
		bss[i].set(SHADOW_BUFFER_SIZE, SHADOW_BUFFER_SIZE, nullptr, s3ds[i].get_arr(), nullptr);
	}
	scene.lights[1].build_shadow_cache(scene, bss, set);

	for (auto i = scene.lights.begin(); i != scene.lights.end(); ++i) {
		if (i->has_shadow) {
			i->set_camera_mat(cammod);
		}
	}
	std::cout << "  done\n";
}
void render_full_fx() {
	std::cout << "rendering full fx...";
	rasterizing::basic_renderer prend;
	rasterizing::rasterizer prast;
	rasterizing::mem_depth_buffer mdb(WND_WIDTH, WND_HEIGHT);
	mem_color_buffer mcb(WND_WIDTH, WND_HEIGHT);
	prast.cur_buf.set(WND_WIDTH, WND_HEIGHT, full_rendering_buf.get_arr(), mdb.get_arr(), nullptr);
	prast.clear_color_buf(device_color(255, 0, 0, 0));
	prast.clear_depth_buf(-1.0);
	mcb.clear(device_color(0, 0, 0, 0));
	prend.scene = &scene;
	prend.cache = &defsc;
	prend.linked_rasterizer = &prast;
	prend.mat_modelview = &cammod;
	prend.mat_projection = &camproj;
	prend.setup_rendering_env();
	long long t1, t2;
	t1 = get_time();
	prend.render_cached();
	render_volumetric_shadow(scene, defsc, camproj, mdb, cam, rasterizing::buffer_set(WND_WIDTH, WND_HEIGHT, mcb.get_arr(), nullptr, nullptr), 20);
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
	enlarged_copy(full_rendering_buf, finalbuf);
	finalbuf.display(wnd.get_dc());
	std::cout << "  done with " << (static_cast<rtt2_float>(get_timer_freq()) / (t2 - t1)) << " fps\n";
}

int main() {
	mem_color_buffer screen_buf(BUF_WIDTH, BUF_HEIGHT);
	rasterizing::mem_depth_buffer mdb(BUF_WIDTH, BUF_HEIGHT);
	stopwatch stw;
	key_monitor incd, decd;

	rast.cur_buf.set(BUF_WIDTH, BUF_HEIGHT, screen_buf.get_arr(), mdb.get_arr(), nullptr);

	cam.hori_fov = 60.0 * RTT2_PI / 180.0;
	cam.aspect_ratio = BUF_HEIGHT / static_cast<rtt2_float>(BUF_WIDTH);
	cam.znear = ZNEAR;
	cam.zfar = ZFAR;
	cam.pos = vec3(INIT_CAM_POS);
	cam.forward = vec3(1.0, 0.0, 0.0);
	cam.up = vec3(0.0, 0.0, 1.0);
	cam.make_cache();

	get_trans_camview_3(cam, cammod);
	get_trans_frustrum_3(cam, camproj);

	rend.linked_rasterizer = &rast;
	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.cache = &defsc;

	initialize_scene();

	incd.set(VK_F1, nullptr, render_shadows);
	decd.set(VK_F2, nullptr, render_shadows);

	wnd.set_client_size(WND_WIDTH, WND_HEIGHT);
	wnd.set_center();
	wnd.show();

	while (goon) {
		rtt2_float delta = stw.tick_in_seconds();

		while (wnd.idle()) {
		}
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);

		if (licam != iscam) {
			std::cout << "\n";
			if (!iscam) {
				render_full_fx();
			}
		}
		licam = iscam;

		incd.update();
		decd.update();

		if (incd.down() != decd.down()) {
			if (incd.down()) {
				test_po.depth += 0.1 * delta;
				std::cout << "depth = " << test_po.depth << "\n";
			} else {
				test_po.depth -= 0.1 * delta;
				std::cout << "depth = " << test_po.depth << "\n";
			}
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
	return 0;
}
