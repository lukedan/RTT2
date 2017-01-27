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

#define WND_WIDTH 800
#define WND_HEIGHT 600

#define MODEL_FILE "rsrc/cornell_box.obj"
#define TEXTURE_FILE "rsrc/tex.ppm"
#define BATCH_SIZE 10000
#define BUF_WIDTH 800
#define BUF_HEIGHT 600

#define ROT_DIV 500.0
#define ZNEAR 0.8
#define INIT_CAM_POS 278.0, 800.0, 273.0
#define MOVE_MULT 10
#define ZFAR 5000.0

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
raytracing::mem_color_accum_buffer rtcbuf(WND_WIDTH, WND_HEIGHT);
raytracing::mem_hitcount_buffer rthbuf(WND_WIDTH, WND_HEIGHT);
raytracing::raytracer tracer;
raytracing::camera_info rtcam;

texture tex, dep;
rasterizing::scene_cache defsc;
rasterizing::scene_description scene;
raytracing::scene_cache tracercache;
raytracing::scene_description raysd;
brdf_diffuse mtrl(1.0);
model_data mdl1;
mat4 mm1;
raytracing::round_planar_light light;
rasterizing::point_light_data l3;

void initialize_scene() {
	std::cout << "loading textures...";
	{
		std::ifstream in(TEXTURE_FILE);
		tex.load_ppm(in);
	}
	tex.make_float_cache_nocheck();

	std::cout << " done\n";

	std::cout << "loading models...";
	{
		std::ifstream in(MODEL_FILE);
		mdl1.load_obj(in);
	}
	if (mdl1.normals.size() == 1) {
		mdl1.generate_normals_flat();
	}
	std::cout << " done\n";

	get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), RTT2_PI * 0.5, mm1);

	std::cout << "setting up lights...";
	l3.pos = vec3(278.0, -279.5, 548.0);
	l3.color = color_vec_rgb(100000.0, 100000.0, 100000.0);

	light.center = vec3(278.0, -279.5, 548.0);
	light.normal = vec3(0.0, 0.0, -1.0);
	//light.center = vec3(250, -220, 460);
	//light.normal = vec3(3.0, 4.0, 5.0);
	//light.normal.set_length(1.0);
	light.radius = 110.0;
	light.illum = color_vec_rgb(10.0, 10.0, 6.0);
	raysd.lights.push_back(&light);
	scene.lights.push_back(rasterizing::light(l3));

	std::cout << " done\n";

	scene.models.push_back(rasterizing::model(&mdl1, &mtrl, &mm1, &tex, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));

	rend.mat_modelview = &cammod;
	rend.mat_projection = &camproj;
	rend.linked_rasterizer = &rast;
	rend.scene = &scene;
	rend.cache = &defsc;
	rend.init_cache();
	rend.refresh_cache();

	raysd.models.push_back(raytracing::model(&mdl1, &mm1, &tex, raytracing::material(&mtrl), color_vec(1.0, 1.0, 1.0, 1.0)));
	tracer.cam = &rtcam;
	tracer.scene = &raysd;
	tracer.cache = &tracercache;
	tracer.build_cache();
}

rtt2_float f_rand() {
	//return std::rand() / static_cast<rtt2_float>(RAND_MAX);
	return dist(eng);
}

void render_polyline(rasterizing::basic_renderer &r, const std::vector<vec3> &pts, const device_color &c) {
	rasterizing::vertex_pos_cache last, cur;
	vec3 dummy;
	for (size_t i = 0; i < pts.size(); ++i) {
		r.linked_rasterizer->shader_vtx(
			*r.linked_rasterizer, *r.mat_modelview,
			pts[i], vec3(0.0, 0.0, 1.0),
			cur.shaded_pos, dummy,
			nullptr
		);
		cur.complete(*r.mat_projection);
		r.linked_rasterizer->cur_buf.denormalize_scr_coord(cur.screen_pos);
		if (i > 0) {
			r.linked_rasterizer->draw_line(last.screen_pos, cur.screen_pos, c);
		}
		last = cur;
	}
}

int main() {
	mem_color_buffer screen_buf(BUF_WIDTH, BUF_HEIGHT);
	rasterizing::mem_depth_buffer mdb(BUF_WIDTH, BUF_HEIGHT);
	stopwatch stw;

	rasterizing::rasterizer dbg_rast;
	rasterizing::basic_renderer dbg_rend;
	std::vector<vec3> dbg_pts;

	dbg_rast.cur_buf.set(WND_WIDTH, WND_HEIGHT, finalbuf.get_arr(), nullptr, nullptr);
	dbg_rend.mat_modelview = &cammod;
	dbg_rend.mat_projection = &camproj;
	dbg_rend.linked_rasterizer = &dbg_rast;
	dbg_rend.setup_compact_rendering_env();

	rast.cur_buf.set(BUF_WIDTH, BUF_HEIGHT, screen_buf.get_arr(), mdb.get_arr(), nullptr);

	size_t rtc = 0;
	tracer.buffer.set(WND_WIDTH, WND_HEIGHT, rtcbuf.get_arr(), rthbuf.get_arr(), full_rendering_buf.get_arr());

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

	wnd.set_client_size(WND_WIDTH, WND_HEIGHT);
	wnd.set_center();
	wnd.show();

	while (goon) {
		rtt2_float delta = stw.tick_in_seconds();

		while (wnd.idle()) {
		}
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);

		if (is_key_down('R')) {
			cam.pos = vec3(INIT_CAM_POS);
			ca_hor = 1.5 * RTT2_PI;
			ca_vert = 0.0;
			cam.forward = vec3(1.0, 0.0, 0.0);
			cam.up = vec3(0.0, 0.0, 1.0);
			cam.make_cache();
		}
		if (licam != iscam) {
			std::cout << "\n";
			if (!iscam) {
				rtc = 0;
				rtcam.set(cam);
				tracer.buffer.clear();
				std::cout << "ray tracing begun\n";
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
			//rend.setup_rendering_env();
			rend.render_cached();

			enlarged_copy(screen_buf, finalbuf);

			if (!is_key_down(VK_RBUTTON)) {
				iscam = false;
			}
		} else {
			int mx, my;
			wnd.get_client_mouse_pos(mx, my);
			if (mx >= 0 && mx < WND_WIDTH && my >= 0 && my < WND_HEIGHT) {
				rtt2_float
					x = mx / static_cast<rtt2_float>(WND_WIDTH),
					y = (WND_HEIGHT - my - 1) / static_cast<rtt2_float>(WND_HEIGHT);
				if (is_key_down(VK_LBUTTON)) {
					dbg_pts = tracer.trace_path_debug(vec2(x, y), f_rand);
				}
				for (size_t i = 0; i < BATCH_SIZE; ++i) {
					tracer.trace_path(vec2(x + (f_rand() - 0.5) * 0.1, y + (f_rand() - 0.5) * 0.1), f_rand);
				}
			} else {
				for (size_t i = 0; i < BATCH_SIZE; ++i) {
					tracer.trace_path(vec2(f_rand(), f_rand()), f_rand);
				}
			}
			rtc += BATCH_SIZE;
			tracer.buffer.flush();
			enlarged_copy(full_rendering_buf, finalbuf);
			std::cout << rtc << "\r";
		}
		render_polyline(dbg_rend, dbg_pts, device_color(255, 255, 255, 255));
		finalbuf.display(wnd.get_dc());
	}
	return 0;
}
