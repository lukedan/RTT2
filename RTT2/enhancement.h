#pragma once

#include "rasterizer.h"

namespace rtt2 {
	namespace rasterizing {
		class enhancement {
		public:
			virtual ~enhancement() {
			}

			virtual size_t get_additional_face_data_size() const = 0;
			virtual void make_face_cache(const rasterizer::vertex_info*, void*) const = 0;
			virtual bool before_test_shader(const rasterizer&, rasterizer::frag_info&, rtt2_float*, unsigned char*, void*) const = 0;
		};

		class parallax_occulusion_mapping_enhancement : public enhancement { // TODO optimization
		public:
			rtt2_float depth = 0.0;
			const texture *depth_tex = nullptr;
		protected:
			struct _vertex_data {
				vec2 uvd12, uvd13;
				vec3 diff12, diff13, uvdx, uvdy;
			};
		public:
			size_t get_additional_face_data_size() const override {
				return sizeof(_vertex_data);
			}
			void make_face_cache(const rasterizer::vertex_info *vi, void *tag) const override {
				_vertex_data *vd = static_cast<_vertex_data*>(tag);
				vd->uvd12 = *vi[1].uv - *vi[0].uv;
				vd->uvd13 = *vi[2].uv - *vi[0].uv;
				vd->diff12 = vi[1].pos->shaded_pos - vi[0].pos->shaded_pos;
				vd->diff13 = vi[2].pos->shaded_pos - vi[0].pos->shaded_pos;
				rtt2_float pv, qv;
				solve_parallelogram_2(vd->uvd12, vd->uvd13, vec2(1.0, 0.0), pv, qv);
				vd->uvdx = vd->diff12 * pv + vd->diff13 * qv;
				solve_parallelogram_2(vd->uvd12, vd->uvd13, vec2(0.0, 1.0), pv, qv);
				vd->uvdy = vd->diff12 * pv + vd->diff13 * qv;
			}
			bool before_test_shader(const rasterizer &rast, rasterizer::frag_info &fi, rtt2_float*, unsigned char*, void *tag) const override {
				_vertex_data *vd = static_cast<_vertex_data*>(tag);
				vec3 fpos = fi.pos3_cache;
				fpos.set_length(1.0);
				rtt2_float sinv = -vec3::dot(fi.normal_cache, fpos), invcosv = 1.0 / std::sqrt(1.0 - sinv * sinv);
				vec3 xd = (fpos + sinv * fi.normal_cache) * invcosv;
				rtt2_float zinc = sinv * invcosv;
				rtt2_float p, q;
				solve_parallelogram_3(vd->diff12, vd->diff13, xd, p, q);
				vec2 pres = p * vd->uvd12 + q * vd->uvd13;
				rtt2_float invlv = 0.5 / (pres.length() * depth_tex->get_w());
				zinc *= invlv;
				pres *= invlv;
				rtt2_float zv = zinc, posmult = invlv * invcosv;
				for (size_t i = 0; i < 200; ++i, fi.uv_cache += pres, fi.pos3_cache += fpos * posmult, zv += zinc) {
					color_vec v;
					sample(*depth_tex, fi.uv_cache, v);
					if ((1.0 - v.x) * depth < zv) {
						break;
					}
				}
				fi.pos4_cache = *rast.mat_proj * vec4(fi.pos3_cache);
				fi.z_cache = fi.pos4_cache.z / fi.pos4_cache.w;

				color_vec gradx, grady;
				depth_tex->grad_x(fi.uv_cache, gradx);
				depth_tex->grad_y(fi.uv_cache, grady);
				rtt2_float uvdxlsq = vd->uvdx.sqr_length(), uvdylsq = vd->uvdy.sqr_length();
				rtt2_float gx = -depth * gradx.x * depth_tex->get_w() / uvdxlsq, gy = -depth * grady.x * depth_tex->get_h() / uvdylsq;
				rtt2_float zmult = std::sqrt(1.0 / (1.0 + gx * gx * uvdxlsq + gy * gy * uvdylsq));
				fi.normal_cache = (fi.normal_cache + vd->uvdx * gx + vd->uvdy * gy) * zmult;

				return true;
			}
		};
	}
}
