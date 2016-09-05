#include <iostream>
#include <sstream>
#include <cfenv>

#include "vec.h"
#include "mat.h"
#include "rasterizer.h"
#include "window.h"
#include "buffer.h"
#include "model.h"

using namespace rtt2;

#define WND_WIDTH 800
#define WND_HEIGHT 600

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
#	define INIT_CAM_X -2.0
#	define ZNEAR 0.8
#	define ZFAR 50.0
#endif

LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);

window wnd("testwnd", wndproc);
bool goon = true, iscam = true;
vec2 mouse(200, 200);

rtt2_float ca_hor = 0.0, ca_vert = 0.0;
vec3 cpos(INIT_CAM_X, 0.0, 0.0);

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
	std::fesetround(FE_DOWNWARD);

	buffer_set buf(WND_WIDTH, WND_HEIGHT);
	rasterizer rast;
	fps_counter counter;
	stopwatch stw;

	buf.make_sys_buffer_set(wnd.get_dc());

	rast.cur_buf = &buf;
	rast.shader_vtx = rasterizer::default_vertex_shader;
	rast.shader_test = rasterizer::default_test_shader;
	rast.shader_frag = rasterizer::default_fragment_shader_notex;

	counter.set_window(0.5);

	wnd.set_client_size(WND_WIDTH, WND_HEIGHT);
	wnd.set_center();
	wnd.show();

	rtt2_float angle = 0.0;
	texture tex;
	{
		file_access in("rsrc/rock.ppm", "r");
		tex.load_ppm(in);
	}
	tex.make_float_cache_nocheck();
	model box;
	{
		file_access in("rsrc/bunny.obj", "r");
		box.load_obj(in);
	}

	mat4 frustrum_mat;
	get_trans_frustrum_3(60.0 * RTT2_PI / 180.0, WND_HEIGHT / static_cast<rtt2_float>(WND_WIDTH), ZNEAR, ZFAR, frustrum_mat);
	rast.mat_proj = &frustrum_mat;

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
		}
		if (is_key_down(VK_SPACE)) {
			angle += 1.0 * delta;
		} else if (is_key_down(VK_RETURN)) {
			angle += 0.1 * delta;
		}
		rtt2_float coshv = std::cos(ca_vert);
		vec3
			forward(std::cos(ca_hor) * coshv, std::sin(ca_hor) * coshv, std::sin(ca_vert)),
			up(vec3::cross(vec3(forward.y, -forward.x, 0.0), forward));
		up.set_length(1.0);
		if (iscam) {
			vec3 right_cache(vec3::cross(forward, up));
			if (is_key_down('W')) {
				cpos += forward * delta * MOVE_MULT;
			}
			if (is_key_down('S')) {
				cpos -= forward * delta * MOVE_MULT;
			}
			if (is_key_down('A')) {
				cpos -= right_cache * delta * MOVE_MULT;
			}
			if (is_key_down('D')) {
				cpos += right_cache * delta * MOVE_MULT;
			}
		}
		mat4 cammat, m, n;
		get_trans_camview_3(cpos, forward, up, cammat);
		get_trans_rotation_3(vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), RTT2_PI * 0.5, m);
		mat4::mult_ref(cammat, m, n);
		rast.mat_modelview = &n;

		for (auto i = box.faces.begin(); i != box.faces.end(); ++i) {
			rast.draw_triangle(
				box.points[i->vertex_ids[0]],
				box.points[i->vertex_ids[1]],
				box.points[i->vertex_ids[2]],
				box.normals[i->normal_ids[0]],
				box.normals[i->normal_ids[1]],
				box.normals[i->normal_ids[2]],
				box.uvs[i->uv_ids[0]],
				box.uvs[i->uv_ids[1]],
				box.uvs[i->uv_ids[2]],
				color_vec(1.0, 1.0, 1.0, 1.0),
				color_vec(1.0, 1.0, 1.0, 1.0),
				color_vec(1.0, 1.0, 1.0, 1.0),
				nullptr
				//&tex
			);
		}

		// end of tests
		buf.display(wnd.get_dc());

		counter.update();
		std::stringstream ss;
		ss << "test fps: " << counter.get_fps() << " / " << counter.get_singleframe_fps();
		wnd.set_caption(ss.str());
	}
	//std::system("pause");
	return 0;
}
