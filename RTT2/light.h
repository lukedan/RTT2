#pragma once

#include "vec.h"
#include "color.h"
#include "buffer.h"

namespace rtt2 {
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

	struct light_data {
		virtual ~light_data() {
		}

		virtual void build_cache(const mat4&, light_cache&) const = 0;
		virtual bool get_illum(const light_cache&, const vec3&, vec3&, color_vec_rgb&) const = 0;
	};

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
	};

	struct scene_cache;
	struct light {
		light() = default;
		light(const light_data &d) : data(&d), shadow(nullptr, nullptr, 0.0) {
		}
		light(const light_data &d, const scene_cache &sc, const buffer_set &buf, rtt2_float tol) : data(&d), shadow(&sc, &buf, tol) {
		}

		const light_data *data;
		struct shadow_cache {
			shadow_cache() = default;
			shadow_cache(const scene_cache *sc, const buffer_set *buf, rtt2_float tol) : scene(sc), buff(buf), tolerance(tol) {
			}

			const scene_cache *scene;
			const buffer_set *buff;
			rtt2_float tolerance;
		} shadow;
	};
}