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

window wnd("testwnd", wndproc);
bool goon = true, iscam = true;
vec2 mouse(200, 200);

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
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int main() {
	buffer_set buf(WND_WIDTH, WND_HEIGHT);
	rasterizer rast;
	fps_counter counter;
	stopwatch stw;
	basic_renderer rend;
	camera_spec cam;
	scene_cache defsc;

	texture tex;
	model_data mdl1, mdl2;
	mat4 mm1, mm2;
	material_phong mtrl(1.0, 1.0, 50.0);

	directional_light l1;
	spotlight l2, l3;

	scene_cache s3sc;
	buffer_set s3bs(5000, 5000);

	buf.make_sys_buffer_set(wnd.get_dc());

	rast.cur_buf = &buf;
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
	cam.forward = vec3(-1.0, 0.0, 0.0);
	cam.up = vec3(0.0, 0.0, 1.0);
	cam.make_cache();

	{
		file_access in("rsrc/tex.ppm", "r");
		tex.load_ppm(in);
	}
	tex.make_float_cache_nocheck();

	{
		file_access in("rsrc/dragon.obj", "r");
		mdl1.load_obj(in);
	}
	if (mdl1.normals.size() == 1) {
		//mdl.generate_normals_flat();
		mdl1.generate_normals_weighted_average();
	}

	mdl2.points.push_back(vec3(-20.0, -20.0, 0.0));
	mdl2.points.push_back(vec3(20.0, -20.0, 0.0));
	mdl2.points.push_back(vec3(-20.0, 20.0, 0.0));
	mdl2.points.push_back(vec3(20.0, 20.0, 0.0));
	mdl2.uvs.push_back(vec2(0.0, 0.0));
	mdl2.uvs.push_back(vec2(1.0, 0.0));
	mdl2.uvs.push_back(vec2(0.0, 1.0));
	mdl2.uvs.push_back(vec2(1.0, 1.0));
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

	l1.color = color_vec_rgb(1.0, 0.3, 0.4);
	l1.dir = vec3(-3.0, -4.0, -5.0);
	l1.dir.set_length(1.0);

	l2.color = color_vec_rgb(10.0, 10.0, 10.0);
	l2.pos = cam.pos;
	l2.dir = cam.forward;
	l2.inner_angle = 0.25;
	l2.outer_angle = 0.3;

	l3.color = color_vec_rgb(9000.0, 9000.0, 9000.0);
	l3.pos = vec3(5.0, -60.0, 95.0);
	l3.dir = vec3(-5.0, 60.0, -95.0);
	l3.dir.set_length(1.0);
	l3.inner_angle = 0.07;
	l3.outer_angle = 0.08;

	rend.cam = &cam;
	rend.linked_rasterizer = &rast;
	rend.models.push_back(model(&mdl1, &mtrl, &mm1, nullptr, color_vec(1.0, 1.0, 1.0, 1.0)));
	rend.models.push_back(model(&mdl2, &mtrl, &mm2, &tex, color_vec(1.0, 1.0, 1.0, 1.0)));
	//rend.lights.push_back(light(l1));
	//rend.lights.push_back(light(l2));
	rend.lights.push_back(light(l3));
	rend.cache = &defsc;
	rend.init_cache();
	rend.refresh_cache();

	{ // build the shadow map
		camera_spec tmp;
		tmp.pos = l3.pos;
		tmp.forward = l3.dir;
		tmp.forward.get_max_prp(tmp.up);
		tmp.make_cache();
		tmp.aspect_ratio = 1.0;
		tmp.hori_fov = 2.0 * l3.outer_angle;
		tmp.znear = 100.0;
		tmp.zfar = 150.0;
		tmp.make_cache();

		rast.cur_buf = &s3bs;
		rast.clear_depth_buf(-1.0);
		rend.cam = &tmp;
		rend.cache = &s3sc;
		rend.init_cache();
		rend.refresh_cache();
		rend.setup_rendering_shadow_env();
		rend.render_cached();

		rend.lights[0].shadow = light::shadow_cache(&s3sc, &s3bs, 0.001);
	}
	rast.cur_buf = &buf;
	rend.cam = &cam;
	rend.cache = &defsc;

	while (goon) {
		rtt2_float delta = stw.tick_in_seconds();

		while (wnd.idle()) {
		}
		rast.clear_color_buf(device_color(255, 0, 0, 0));
		rast.clear_depth_buf(-1.0);

		bool nic = is_key_down(VK_RBUTTON);
		if (nic && nic != iscam) {
			wnd.put_mouse_to_center();
		}
		iscam = nic;
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

			l2.pos = cam.pos;
			l2.dir = cam.forward;

			rend.refresh_cache();
		}

		rend.setup_rendering_env();
		rend.render_cached();

		// end of tests
		buf.display(wnd.get_dc());

		counter.update();
		std::stringstream ss;
		ss << "rasterizer test | fps: " << counter.get_fps();
		wnd.set_caption(ss.str());
	}
	//std::system("pause");
	return 0;
}
