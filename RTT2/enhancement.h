#pragma once

#include "rasterizer.h"

namespace rtt2 {
	class enhancement {
	public:
		virtual ~enhancement() {
		}

		virtual bool on_test_shader(const rasterizer&, rasterizer::frag_info&, rtt2_float*, unsigned char*, void*) const = 0;
	};

	class parallax_occulusion_mapping_enhancement : public enhancement { // TODO optimization
	public:
		rtt2_float depth;
		const texture *depth_tex;

		bool on_test_shader(const rasterizer &rast, rasterizer::frag_info &fi, rtt2_float*, unsigned char*, void*) const override {
			vec3 fpos = fi.pos3_cache;
			fpos.set_length(1.0);
			rtt2_float sinv = -vec3::dot(fi.normal_cache, fpos), cosv = std::sqrt(1.0 - sinv * sinv);
			rtt2_float zinc = sinv / cosv;
			vec3 yd, xd;
			vec3::cross_ref(fi.normal_cache, fpos, yd);
			yd.set_length(1.0);
			vec3::cross_ref(yd, fi.normal_cache, xd);
			vec3 d1 = fi.v[1].pos->shaded_pos - fi.v[0].pos->shaded_pos, d2 = fi.v[2].pos->shaded_pos - fi.v[0].pos->shaded_pos;
			rtt2_float p, q;
			solve_linear_2(d1.xy(), d2.xy(), xd.xy(), p, q);
			vec2 uvd1 = *fi.v[1].uv - *fi.v[0].uv, uvd2 = *fi.v[2].uv - *fi.v[0].uv;
			vec2 pres = p * uvd1 + q * uvd2;
			rtt2_float lv = 2.0 * pres.length() * depth_tex->get_w();
			zinc /= lv;
			pres /= lv;
			rtt2_float zv = zinc;
			for (size_t i = 0; i < 200; ++i, fi.uv_cache += pres, fi.pos3_cache += fpos / (lv * cosv), zv += zinc) {
				color_vec v;
				depth_tex->sample(fi.uv_cache, v);
				if ((1.0 - v.x) * depth < zv) {
					break;
				}
			}
			fi.pos4_cache = *rast.mat_proj * vec4(fi.pos3_cache);
			fi.z_cache = fi.pos4_cache.z / fi.pos4_cache.w;

			rtt2_float npx, nqx, npy, nqy;
			solve_linear_2(uvd1, uvd2, vec2(1.0, 0.0), npx, nqx);
			vec3 uvdx = npx * d1 + nqx * d2;
			solve_linear_2(uvd1, uvd2, vec2(0.0, 1.0), npy, nqy);
			vec3 uvdy = npy * d1 + nqy * d2;
			color_vec gradx, grady;
			depth_tex->grad_x(fi.uv_cache, gradx);
			depth_tex->grad_y(fi.uv_cache, grady);
			rtt2_float gx = -depth * gradx.x * depth_tex->get_w() / uvdx.length(), gy = -depth * grady.x * depth_tex->get_h() / uvdy.length();
			rtt2_float zmult = std::sqrt(1.0 / (1.0 + gx * gx + gy * gy));
			fi.normal_cache = (fi.normal_cache + uvdx * gx / uvdx.length() + uvdy * gy / uvdy.length()) * zmult;

			//fi.color_mult_cache = vec4(fi.normal_cache * 0.5 + vec3(0.5, 0.5, 0.5));

			return true;
		}
	};
}
