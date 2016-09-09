#pragma once

#include "vec.h"
#include "color.h"
#include "buffer.h"

namespace rtt2 {
	class basic_renderer;
	struct light_data;

	union light_cache {
		struct {
			vec3 pos;
		} of_pointlight;
		struct {
			vec3 dir;
		} of_directional;
		struct {
			vec3 pos, dir;
			rtt2_float inner_cosv, outer_cosv;
		} of_spotlight;
	};
	union shadow_settings {
		struct {
			rtt2_float tolerance, znear, zfar;
		} of_spotlight;
	};

	struct light {
		light() = default;
		light(const light_data &d) : data(&d) {
		}

		const light_data *data;
		bool has_shadow = false;
		union shadow_data {
			struct {
				void set(const mat4 &mv, const mat4 &pj, const buffer_set &buf, rtt2_float tol) {
					mat4::mult_ref(pj, mv, mat_w2sm);
					buff = buf;
					tolerance = tol;
				}

				mat4 mat_w2sm, mat_tot;
				buffer_set buff;
				rtt2_float tolerance;
			} of_spotlight;
		} shadow_cache;
		void set_inverse_camera_mat(const mat4&);
		bool in_shadow(const vec3&) const;
		void build_shadow_cache(basic_renderer&, const buffer_set&, const shadow_settings&);
	};

	struct light_data {
		virtual ~light_data() {
		}

		virtual void build_cache(const mat4&, light_cache&) const = 0;
		virtual bool get_illum(const light_cache&, const vec3&, vec3&, color_vec_rgb&) const = 0;

		virtual void build_shadow_cache(basic_renderer&, const buffer_set&, const shadow_settings&, light::shadow_data&) const = 0;
		virtual void set_inverse_camera_mat(const mat4&, light::shadow_data&) const = 0;
		virtual bool in_shadow(const vec3&, const light::shadow_data&) const = 0;
	};

	inline void light::set_inverse_camera_mat(const mat4 &mt) {
		data->set_inverse_camera_mat(mt, shadow_cache);
	}
	inline bool light::in_shadow(const vec3 &pt) const {
		return (has_shadow ? data->in_shadow(pt, shadow_cache) : false);
	}
	inline void light::build_shadow_cache(basic_renderer &rend, const buffer_set &bs, const shadow_settings &set) {
		data->build_shadow_cache(rend, bs, set, shadow_cache);
		has_shadow = true;
	}

	struct pointlight : public light_data {
		pointlight() = default;
		pointlight(const vec3 &p, const color_vec_rgb &c) : pos(p), color(c) {
		}

		vec3 pos;
		color_vec_rgb color;

		void build_cache(const mat4 &m, light_cache &res) const override {
			transform_default(m, pos, res.of_pointlight.pos);
		}
		bool get_illum(const light_cache &c, const vec3 &fpos, vec3 &idir, color_vec_rgb &res) const override {
			idir = fpos - c.of_pointlight.pos;
			rtt2_float invsql = 1.0 / idir.sqr_length();
			res = color * invsql;
			idir *= std::sqrt(invsql);
			return true;
		}
	};
	struct directional_light : public light_data {
		vec3 dir;
		color_vec_rgb color;

		void build_cache(const mat4 &m, light_cache &c) const override {
			transform_default(m, dir, c.of_directional.dir, 0.0);
			c.of_directional.dir.set_length(1.0);
		}
		bool get_illum(const light_cache &cac, const vec3&, vec3 &idir, color_vec_rgb &res) const override {
			idir = cac.of_directional.dir;
			res = color;
			return true;
		}
	};
	struct spotlight : public light_data {
		vec3 pos, dir;
		color_vec_rgb color;
		rtt2_float inner_angle, outer_angle;

		void build_cache(const mat4 &m, light_cache &c) const override {
			transform_default(m, pos, c.of_spotlight.pos);
			transform_default(m, dir, c.of_spotlight.dir, 0.0);
			c.of_spotlight.dir.set_length(1.0);
			c.of_spotlight.inner_cosv = std::cos(inner_angle);
			c.of_spotlight.outer_cosv = std::cos(outer_angle);
		}
		bool get_illum(const light_cache &c, const vec3 &fpos, vec3 &rdir, color_vec_rgb &res) const override {
			rdir = fpos - c.of_spotlight.pos;
			rtt2_float invsql = 1.0 / rdir.sqr_length();;
			rdir *= std::sqrt(invsql);
			rtt2_float dotv = vec3::dot(rdir, c.of_spotlight.dir);
			if (dotv < c.of_spotlight.outer_cosv) {
				return false;
			}
			rtt2_float mult = std::min(1.0, (dotv - c.of_spotlight.outer_cosv) / (c.of_spotlight.inner_cosv - c.of_spotlight.outer_cosv));
			res = color * invsql * mult;
			return true;
		}

		void build_shadow_cache(basic_renderer&, const buffer_set&, const shadow_settings&, light::shadow_data&) const override;
		void set_inverse_camera_mat(const mat4 &mt, light::shadow_data &data) const override {
			mat4::mult_ref(data.of_spotlight.mat_w2sm, mt, data.of_spotlight.mat_tot);
		}
		bool in_shadow(const vec3 &pt, const light::shadow_data &data) const override {
			vec4 cp = data.of_spotlight.mat_tot * vec4(pt);
			vec2 xy;
			cp.homogenize_2(xy);
			data.of_spotlight.buff.denormalize_scr_coord(xy);
			size_t
				x = static_cast<size_t>(clamp(xy.x, 0.5, data.of_spotlight.buff.w - 0.5)),
				y = static_cast<size_t>(clamp(xy.y, 0.5, data.of_spotlight.buff.h - 0.5));
			rtt2_float *z = data.of_spotlight.buff.get_at(x, y, data.of_spotlight.buff.depth_arr);
			return cp.z / cp.w < *z - data.of_spotlight.tolerance;
		}
	};
}
