#pragma once

#include <algorithm>
#include <stdexcept>
#include <fstream>

#include "utils.h"
#include "vec.h"
#include "buffer.h"
#include "texture.h"
#include "brdf.h"

namespace rtt2 {
	class rasterizer {
	public:
		rasterizer() = default;
		rasterizer(const rasterizer&) = delete;
		rasterizer &operator =(const rasterizer&) = delete;

		buffer_set *cur_buf;

		void clear_color_buf(const device_color &c) {
			device_color *cur = cur_buf->get_color_arr();
			for (size_t i = cur_buf->get_w() * cur_buf->get_h(); i > 0; --i, ++cur) {
				*cur = c;
			}
		}
		void clear_depth_buf(rtt2_float v) {
			rtt2_float *cur = cur_buf->get_depth_arr();
			for (size_t i = cur_buf->get_w() * cur_buf->get_h(); i > 0; --i, ++cur) {
				*cur = v;
			}
		}

		void set_pixel(size_t x, size_t y, const device_color &c) {
#ifdef DEBUG
			if (x >= cur_buf->get_w() || y >= cur_buf->get_h()) {
				throw std::range_error("target pixel out of boundary");
			}
#endif
			*get_color_buf_at(x, y) = c;
		}
		void get_pixel(size_t x, size_t y, device_color &c) {
#ifdef DEBUG
			if (x >= cur_buf->get_w() || y >= cur_buf->get_h()) {
				throw std::range_error("target pixel out of boundary");
			}
#endif
			c = *get_color_buf_at(x, y);
		}

	protected:
		void draw_line_right(rtt2_float fx, rtt2_float fy, rtt2_float tx, rtt2_float k, const device_color &c) { // handles horizontal clipping
			size_t t = static_cast<size_t>(clamp<rtt2_float>(tx, 0.0, cur_buf->get_w() - 1));
			tx -= fx;
			fx -= 0.5;
			for (size_t cx = static_cast<size_t>(std::max(fx + 0.5, 0.0)); cx <= t; ++cx) {
#ifdef DEBUG
				rtt2_float y = fy + k * clamp(cx - fx, 0.0, tx);
				if (y < 0.0 || y > cur_buf->get_h()) {
					throw std::range_error("vertical clipping incorrect");
				}
#endif
				set_pixel(cx, static_cast<size_t>(fy + k * clamp(cx - fx, 0.0, tx)), c);
			}
		}
		void draw_line_up(rtt2_float by, rtt2_float bx, rtt2_float ty, rtt2_float invk, const device_color &c) { // handles vertical clipping
			size_t t = static_cast<size_t>(clamp<rtt2_float>(ty, 0.0, cur_buf->get_h() - 1));
			ty -= by;
			by -= 0.5;
			for (size_t cy = static_cast<size_t>(std::max(by + 0.5, 0.0)); cy <= t; ++cy) {
#ifdef DEBUG
				rtt2_float x = bx + invk * clamp(cy - by, 0.0, ty);
				if (x < 0.0 || x > cur_buf->get_w()) {
					throw std::range_error("horizontal clipping incorrect");
				}
#endif
				set_pixel(static_cast<size_t>(bx + invk * clamp(cy - by, 0.0, ty)), cy, c);
			}
		}
	public:
		void draw_line(const vec2 &p1, const vec2 &p2, const device_color &c) {
			vec2 pf, pt;
			if (p1.x + p1.y > p2.x + p2.y) {
				pf = p2;
				pt = p1;
			} else {
				pf = p1;
				pt = p2;
			}
			vec2 diff(pt - pf);
			if (diff.x < 0.0 ? true : (diff.y < 0.0 ? false : (std::fabs(diff.y) > std::fabs(diff.x)))) { // messy
				if (clip_line_onedir(pf.x, pf.y, pt.x, pt.y, 0.5, cur_buf->get_w() - 0.5)) {
					draw_line_up(pf.y, pf.x, pt.y, diff.x / diff.y, c);
				}
			} else {
				if (clip_line_onedir(pf.y, pf.x, pt.y, pt.x, 0.5, cur_buf->get_h() - 0.5)) {
					draw_line_right(pf.x, pf.y, pt.x, diff.y / diff.x, c);
				}
			}
		}

	protected:
		struct vertex_info {
			vertex_info() = default;
			vertex_info(
				const vec4 &pp,
				const vec3 &p3,
				const vec3 &normal,
				const vec2 &uvv,
				const color_vec &cc
			) : rp(&pp), tp(&p3), n(&normal), uv(&uvv), c(&cc) {
			}

			const vec4 *rp;
			const vec3 *tp, *n;
			const vec2 *uv;
			const color_vec *c;
		};
	public:
		struct frag_info {
			rtt2_float p, q, r;
			unsigned char stencil;
			const vertex_info *v;
			vec4 pos4_cache;
			rtt2_float z_cache;

#define RTT2_FRAG_INFO_INTERPOLATE(FIELD) (*v[0].FIELD * p + *v[1].FIELD * q + *v[2].FIELD * r)

			void make_cache() {
				pos4_cache = RTT2_FRAG_INFO_INTERPOLATE(rp);
				z_cache = pos4_cache.z / pos4_cache.w;
			}

			vec2 get_uv() const {
				return RTT2_FRAG_INFO_INTERPOLATE(uv);
			}
			vec3 get_pos3() const {
				return RTT2_FRAG_INFO_INTERPOLATE(tp);
			}
			vec3 get_normal_not_normalized() const {
				return RTT2_FRAG_INFO_INTERPOLATE(n);
			}
			vec3 get_normal() const {
				vec3 res(get_normal_not_normalized());
				res.set_length(1.0);
				return res;
			}
			color_vec get_color_mult() const {
				return RTT2_FRAG_INFO_INTERPOLATE(c);
			}
		};
		struct fix_proj_params {
			rtt2_float
				div_c, div_xm, div_ym,
				m11c, m11ym, m12c, m12xm,
				m21c, m21ym, m22c, m22xm,
				vxc, vyc, vm;
		};

		typedef void(*vertex_shader)(const rasterizer&, const mat4&, const vec3&, const vec3&, vec3&, vec3&);
		typedef bool(*test_shader)(const rasterizer&, const frag_info&, rtt2_float&, unsigned char&);
		typedef void(*fragment_shader)(const rasterizer&, const frag_info&, const texture*, device_color&);

		inline static void default_vertex_shader(const rasterizer&, const mat4 &mat, const vec3 &pos, const vec3 &n, vec3 &resr, vec3 &resn) {
			transform_default(mat, pos, resr);
			transform_default(mat, n, resn, 0.0);
		}
		inline static bool default_test_shader(
			const rasterizer&,
			const frag_info &fi,
			rtt2_float &zres,
			unsigned char&
		) {
			if (fi.z_cache > zres) {
				zres = fi.z_cache;
				return true;
			}
			return false;
		}
		inline static void default_fragment_shader_withtex(const rasterizer&, const frag_info &info, const texture *tex, device_color &res) {
			vec3 pos(info.get_pos3()), np = pos;
			color_vec_rgb c, clres;
			c.x = c.y = c.z = 10.0 / pos.sqr_length();
			np.set_length(1.0);
			brdf_phong(np, -np, info.get_normal(), c, material_phong(1.0, 0.5, 50.0), clres);
			color_vec tv;
			tex->sample(info.get_uv(), tv, uv_clamp_mode::repeat_border, sample_mode::bilinear);
			color_vec_mult(tv, info.get_color_mult(), tv);
			color_vec_mult(tv, vec4(clres), tv);
			clamp_vec(tv, 0.0, 1.0);
			res.from_vec4(tv);
		}
		inline static void default_fragment_shader_notex(const rasterizer&, const frag_info &info, const texture*, device_color &res) {
			vec3 pos(info.get_pos3()), np = pos, xxx = pos - vec3(3, 4, 5);
			color_vec_rgb c, clres;
			c.x = c.y = c.z = 10.0 / pos.sqr_length();
			np.set_length(1.0);
			xxx.set_length(1.0);
			brdf_phong(xxx, -np, info.get_normal(), c, material_phong(1.0, 0.5, 50.0), clres);

			color_vec tv = info.get_color_mult();

			color_vec_mult(tv, vec4(clres), tv);
			clamp_vec(tv, 0.0, 1.0);

			res.from_vec4(tv);
		}

		void fix_proj_tex_mapping(const fix_proj_params &params, rtt2_float x, rtt2_float y, rtt2_float &p, rtt2_float &q) const {
			rtt2_float
				vx = params.vxc + params.vm * x, vy = params.vyc + params.vm * y,
				dm = 1.0 / (params.div_c + params.div_xm * x + params.div_ym * y);
			p = ((params.m11c + params.m11ym * y) * vx + (params.m12c + params.m12xm * x) * vy) * dm;
			q = ((params.m21c + params.m21ym * y) * vx + (params.m22c + params.m22xm * x) * vy) * dm;
		}
		void get_fix_proj_params(const vec4 &p1, const vec4 &p2, const vec4 &p3, fix_proj_params &params) {
			vec4 v12 = p2 - p1, v23 = p3 - p2, v31 = p1 - p3;
			params.div_c = p1.y * v23.x + p2.y * v31.x + p3.y * v12.x;
			params.div_xm = p1.w * v23.y + p2.w * v31.y + p3.w * v12.y;
			params.div_ym = -p1.w * v23.x - p2.w * v31.x - p3.w * v12.x;
			params.m11c = -v23.y;
			params.m11ym = v23.w;
			params.m12c = v23.x;
			params.m12xm = -v23.w;
			params.m21c = -v31.y;
			params.m21ym = v31.w;
			params.m22c = v31.x;
			params.m22xm = -v31.w;
			params.vxc = -p3.x;
			params.vyc = -p3.y;
			params.vm = p3.w;
		}

	protected:
		// handles all clipping work
		void draw_half_triangle_half(
			rtt2_float sx, rtt2_float sy, rtt2_float invk1, rtt2_float invk2, rtt2_float ymin, rtt2_float ymax,
			const fix_proj_params &params, const vertex_info *v, const texture *tex
		) {
			sx += 0.5;
			sy -= 0.5;
			size_t
				miny = static_cast<size_t>(std::max(ymin + 0.5, 0.0)),
				maxy = static_cast<size_t>(clamp<rtt2_float>(ymax + 0.5, 0.0, cur_buf->get_h()));
			rtt2_float xstep = 2.0 / cur_buf->get_w(), ystep = 2.0 / cur_buf->get_h(), ys = (miny + 0.5) * ystep - 1.0;
			for (size_t y = miny; y < maxy; ++y, ys += ystep) {
				rtt2_float diff = y - sy, left = diff * invk1 + sx, right = diff * invk2 + sx;
				size_t
					l = static_cast<size_t>(std::max(left, 0.0)),
					r = static_cast<size_t>(clamp<rtt2_float>(right, 0.0, cur_buf->get_w()));
				rtt2_float xs = (l + 0.5) * xstep - 1.0;
				fragment_data frag;
				get_fragment_data_at(l, y, frag);
				for (size_t cx = l; cx < r; ++cx, xs += xstep, frag.incr()) {
					frag_info fi;
					fix_proj_tex_mapping(params, xs, ys, fi.p, fi.q);
					fi.r = 1.0 - fi.p - fi.q;
					fi.v = v;
					fi.make_cache();
					if (shader_test(*this, fi, *frag.z, *frag.stencil)) {
						shader_frag(*this, fi, tex, *frag.color);
					}
				}
			}
		}
#ifdef DEBUG
		void draw_triangle_debug(const vec2 &p1, const vec2 &p2, const vec2 &p3) {
			rtt2_float
				l = std::min({ p1.x, p2.x, p3.x }),
				r = std::max({ p1.x, p2.x, p3.x }),
				u = std::max({ p1.y, p2.y, p3.y }),
				d = std::min({ p1.y, p2.y, p3.y });
			for (size_t y = static_cast<size_t>(d + 0.5); y < static_cast<size_t>(u + 0.5); ++y) {
				for (size_t x = static_cast<size_t>(l + 0.5); x < static_cast<size_t>(r + 0.5); ++x) {
					bool in =
						vec2::cross(vec2(x + 0.5, y + 0.5) - p1, p2 - p1) < 0.0 &&
						vec2::cross(vec2(x + 0.5, y + 0.5) - p2, p3 - p2) < 0.0 &&
						vec2::cross(vec2(x + 0.5, y + 0.5) - p3, p1 - p3) < 0.0;
					if (in) {
						device_color ctmp;
						get_pixel(x, y, ctmp);
						if (ctmp.get_r() > 128) {
							set_pixel(x, y, device_color(255, 100, 255, 100));
						} else {
							set_pixel(x, y, device_color(255, 255, 0, 0));
						}
					}
				}
			}
		}
#endif
		void draw_triangle_impl(const vertex_info *v, const vec2 *rposs, const texture *tex) {
#ifdef RTT2_FORCE_WIREFRAME
			device_color c;
			c.from_vec4(*v[0].c);
			draw_line(rposs[0], rposs[1], c);
			draw_line(rposs[1], rposs[2], c);
			draw_line(rposs[2], rposs[0], c);
#else
			const vec2 *pscrp[3]{ &rposs[0], &rposs[1], &rposs[2] };
			RTT2_SORT3(pscrp, ->y, < );
			rtt2_float
				invk_tm = (pscrp[1]->x - pscrp[0]->x) / (pscrp[1]->y - pscrp[0]->y),
				invk_td = (pscrp[2]->x - pscrp[0]->x) / (pscrp[2]->y - pscrp[0]->y),
				invk_md = (pscrp[2]->x - pscrp[1]->x) / (pscrp[2]->y - pscrp[1]->y);
			fix_proj_params params;
			get_fix_proj_params(*v[0].rp, *v[1].rp, *v[2].rp, params);
			if (invk_tm < invk_td) {
				draw_half_triangle_half(pscrp[0]->x, pscrp[0]->y, invk_td, invk_tm, pscrp[1]->y, pscrp[0]->y, params, v, tex);
			} else {
				draw_half_triangle_half(pscrp[0]->x, pscrp[0]->y, invk_tm, invk_td, pscrp[1]->y, pscrp[0]->y, params, v, tex);
			}
			if (invk_td < invk_md) {
				draw_half_triangle_half(pscrp[2]->x, pscrp[2]->y, invk_td, invk_md, pscrp[2]->y, pscrp[1]->y, params, v, tex);
			} else {
				draw_half_triangle_half(pscrp[2]->x, pscrp[2]->y, invk_md, invk_td, pscrp[2]->y, pscrp[1]->y, params, v, tex);
			}
#endif
	}

		void clip_against_xy(const vec4 &front, const vec4 &back, vec2 &resp) {
			(front * back.z - back * front.z).homogenize_2(resp);
		}
	public:
		void draw_triangle(
			const vec3 &p1, const vec3 &p2, const vec3 &p3,
			const vec3 &n1, const vec3 &n2, const vec3 &n3,
			const vec2 &uv1, const vec2 &uv2, const vec2 &uv3,
			const color_vec &c1, const color_vec &c2, const color_vec &c3,
			const texture *tex
		) {
			vec3 pp1, pp2, pp3, pn1, pn2, pn3;
			shader_vtx(*this, *mat_modelview, p1, n1, pp1, pn1);
			shader_vtx(*this, *mat_modelview, p2, n2, pp2, pn2);
			shader_vtx(*this, *mat_modelview, p3, n3, pp3, pn3);
			if (vec3::dot(pp1, vec3::cross(pp2, pp3)) > 0.0) {
				return;
			}
			vec4 r1, r2, r3;
			r1 = *mat_proj * vec4(pp1);
			r2 = *mat_proj * vec4(pp2);
			r3 = *mat_proj * vec4(pp3);
			vec2 hgzd[3];
			vertex_info v[3]{
				{ r1, pp1, pn1, uv1, c1 },
				{ r2, pp2, pn2, uv2, c2 },
				{ r3, pp3, pn3, uv3, c3 }
			};
			RTT2_SORT3(v, .rp->z, < );
			if (v[2].rp->z > 0.0) {
				return;
			} else if (v[1].rp->z > 0.0) {
				clip_against_xy(*v[2].rp, *v[0].rp, hgzd[0]);
				clip_against_xy(*v[2].rp, *v[1].rp, hgzd[1]);
				v[2].rp->homogenize_2(hgzd[2]);
				for (size_t i = 0; i < 3; ++i) {
					denormalize_scr_coord(hgzd[i]);
				}
				draw_triangle_impl(v, hgzd, tex);
			} else if (v[0].rp->z > 0.0) {
				clip_against_xy(*v[1].rp, *v[0].rp, hgzd[0]);
				v[1].rp->homogenize_2(hgzd[1]);
				v[2].rp->homogenize_2(hgzd[2]);
				for (size_t i = 0; i < 3; ++i) {
					denormalize_scr_coord(hgzd[i]);
				}
				draw_triangle_impl(v, hgzd, tex);

				clip_against_xy(*v[2].rp, *v[0].rp, hgzd[1]);
				denormalize_scr_coord(hgzd[1]);
				draw_triangle_impl(v, hgzd, tex);
			} else {
				for (size_t i = 0; i < 3; ++i) {
					v[i].rp->homogenize_2(hgzd[i]);
					denormalize_scr_coord(hgzd[i]);
				}
				draw_triangle_impl(v, hgzd, tex);
			}
		}

		void denormalize_scr_coord(rtt2_float &x, rtt2_float &y) const {
			x = (x + 1) * 0.5 * cur_buf->get_w();
			y = (y + 1) * 0.5 * cur_buf->get_h();
		}
		void denormalize_scr_coord(vec2 &r) const {
			denormalize_scr_coord(r.x, r.y);
		}
		void normalize_scr_coord(size_t x, size_t y, rtt2_float &rx, rtt2_float &ry) const {
			rx = (2 * x + 1) / static_cast<rtt2_float>(cur_buf->get_w()) - 1.0;
			ry = (2 * y + 1) / static_cast<rtt2_float>(cur_buf->get_h()) - 1.0;
		}
		void normalize_scr_coord(size_t x, size_t y, vec2 &r) const {
			normalize_scr_coord(x, y, r.x, r.y);
		}

		vertex_shader shader_vtx;
		test_shader shader_test;
		fragment_shader shader_frag;

		mat4 *mat_proj, *mat_modelview;
	protected:
		struct fragment_data {
			device_color *color;
			rtt2_float *z;
			unsigned char *stencil;

			void incr() {
				++color;
				++z;
				++stencil;
			}
			void decr() {
				--color;
				--z;
				--stencil;
			}
		};
		device_color *get_color_buf_at(size_t x, size_t y) const {
			return cur_buf->get_color_arr() + (cur_buf->get_w() * y + x);
		}
		void get_fragment_data_at(size_t x, size_t y, fragment_data &data) const {
			size_t id = cur_buf->get_w() * y + x;
			data.color = cur_buf->get_color_arr() + id;
			data.z = cur_buf->get_depth_arr() + id;
			data.stencil = cur_buf->get_stencil_arr() + id;
		}
};
}