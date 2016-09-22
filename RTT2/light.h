#pragma once

#include "vec.h"
#include "color.h"
#include "buffer.h"

namespace rtt2 {
	struct scene_description;
	struct light_data;
	class rasterizer;

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
		struct {
			rtt2_float tolerance, znear, zfar;
		} of_pointlight;
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
			struct {
				void set(const buffer_set *bs, rtt2_float tol) {
					std::memcpy(buff, bs, sizeof(buff));
					tolerance = tol;
				}

				mat4 mat_invcam, mat_proj;
				buffer_set buff[6];
				rtt2_float tolerance;
			} of_pointlight;
		} shadow_cache;
		void set_camera_mat(const mat4&);
		bool in_shadow(const vec3&) const;
		void build_shadow_cache(const scene_description&, const buffer_set*, const shadow_settings&);
		void build_shadow_cache_async(const scene_description&, const buffer_set*, const shadow_settings&, task_lock&);
	};

	struct light_data {
		virtual ~light_data() {
		}

		virtual void build_cache(const mat4&, light_cache&) const = 0;
		virtual bool get_illum(const light_cache&, const vec3&, vec3&, color_vec_rgb&) const = 0;

		virtual void build_shadow_cache(const scene_description&, const buffer_set*, const shadow_settings&, light::shadow_data&) const = 0;
		virtual void build_shadow_cache_async(const scene_description&, const buffer_set*, const shadow_settings&, light::shadow_data&, task_lock&) const = 0;
		virtual void set_camera_mat(const mat4&, light::shadow_data&) const = 0;
		virtual bool in_shadow(const vec3&, const light::shadow_data&) const = 0;
	};

	inline void light::set_camera_mat(const mat4 &mt) {
		data->set_camera_mat(mt, shadow_cache);
	}
	inline bool light::in_shadow(const vec3 &pt) const {
		return (has_shadow ? data->in_shadow(pt, shadow_cache) : false);
	}
	inline void light::build_shadow_cache(const scene_description &sc, const buffer_set *bs, const shadow_settings &set) {
		data->build_shadow_cache(sc, bs, set, shadow_cache);
		has_shadow = true;
	}
	inline void light::build_shadow_cache_async(const scene_description &sc, const buffer_set *bs, const shadow_settings &set, task_lock &lock) {
		data->build_shadow_cache_async(sc, bs, set, shadow_cache, lock);
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
		void build_shadow_cache(const scene_description&, const buffer_set*, const shadow_settings&, light::shadow_data&) const override;
		void build_shadow_cache_async(const scene_description&, const buffer_set*, const shadow_settings&, light::shadow_data&, task_lock&) const override;
		void set_camera_mat(const mat4 &mt, light::shadow_data &data) const override {
			mt.get_inversion(data.of_pointlight.mat_invcam);
		}
		bool in_shadow(const vec3 &pt, const light::shadow_data &data) const override {
			vec3 t, res;
			transform_default(data.of_pointlight.mat_invcam, pt, t);
			t -= pos;
			size_t face = get_pos_info(t, res);
			vec4 cp = data.of_pointlight.mat_proj * vec4(res);
			vec2 xy;
			cp.homogenize_2(xy);
			data.of_pointlight.buff[face].denormalize_scr_coord(xy);
			size_t
				x = static_cast<size_t>(clamp(xy.x, 0.5, data.of_pointlight.buff[face].w - 0.5)),
				y = static_cast<size_t>(clamp(xy.y, 0.5, data.of_pointlight.buff[face].h - 0.5));
			rtt2_float *z = data.of_pointlight.buff[face].get_at(x, y, data.of_pointlight.buff[face].depth_arr), zv = cp.z / cp.w;
			return zv > -1.0 && zv < *z - data.of_pointlight.tolerance;
		}
		/*     +---+
		**     | 5 |   0: -z
		** +---+---+---+---+
		** | 2 | 0 | 3 | 1 |
		** +---+---+---+---+
		**     | 4 |
		**     +---+
		*/
		inline static size_t get_pos_info(const vec3 &dir, vec3 &cd) {
			vec3 aaa(std::abs(dir.x), std::abs(dir.y), std::abs(dir.z));
			if (aaa.z > aaa.x && aaa.z > aaa.y) {
				if (dir.z < 0.0) {
					rot_cubemap<0>(dir, cd);
					return 0;
				}
				rot_cubemap<1>(dir, cd);
				return 1;
			} else if (aaa.x > aaa.y) {
				if (dir.x < 0.0) {
					rot_cubemap<2>(dir, cd);
					return 2;
				}
				rot_cubemap<3>(dir, cd);
				return 3;
			} else {
				if (dir.y < 0.0) {
					rot_cubemap<4>(dir, cd);
					return 4;
				}
				rot_cubemap<5>(dir, cd);
				return 5;
			}
		}
		template <size_t Side> inline static void rot_cubemap(const vec3 &v, vec3 &r) {
			r = v;
		}
		template <> inline static void rot_cubemap<1>(const vec3 &v, vec3 &r) {
			r.x = -v.x;
			r.y = v.y;
			r.z = -v.z;
		}
		template <> inline static void rot_cubemap<2>(const vec3 &v, vec3 &r) {
			r.x = -v.z;
			r.y = v.y;
			r.z = v.x;
		}
		template <> inline static void rot_cubemap<3>(const vec3 &v, vec3 &r) {
			r.x = v.z;
			r.y = v.y;
			r.z = -v.x;
		}
		template <> inline static void rot_cubemap<4>(const vec3 &v, vec3 &r) {
			r.x = v.x;
			r.y = -v.z;
			r.z = v.y;
		}
		template <> inline static void rot_cubemap<5>(const vec3 &v, vec3 &r) {
			r.x = v.x;
			r.y = v.z;
			r.z = -v.y;
		}
		template <size_t Side> inline static void set_rot_mat(mat4 &m) {
			m.set_identity();
		}
		template <> inline static void set_rot_mat<1>(mat4 &m) {
			m.set_zero();
			m[0][0] = m[2][2] = -1.0;
			m[1][1] = m[3][3] = 1.0;
		}
		template <> inline static void set_rot_mat<2>(mat4 &m) {
			m.set_zero();
			m[2][0] = -1.0;
			m[1][1] = m[0][2] = m[3][3] = 1.0;
		}
		template <> inline static void set_rot_mat<3>(mat4 &m) {
			m.set_zero();
			m[2][0] = m[1][1] = m[3][3] = 1.0;
			m[0][2] = -1.0;
		}
		template <> inline static void set_rot_mat<4>(mat4 &m) {
			m.set_zero();
			m[0][0] = m[1][2] = m[3][3] = 1.0;
			m[2][1] = -1.0;
		}
		template <> inline static void set_rot_mat<5>(mat4 &m) {
			m.set_zero();
			m[0][0] = m[2][1] = m[3][3] = 1.0;
			m[1][2] = -1.0;
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

		void build_shadow_cache(const scene_description&, const buffer_set*, const shadow_settings&, light::shadow_data&) const override;
		void build_shadow_cache_async(const scene_description&, const buffer_set*, const shadow_settings&, light::shadow_data&, task_lock&) const override;
		void set_camera_mat(const mat4 &mt, light::shadow_data &data) const override {
			mat4 inv;
			mt.get_inversion(inv);
			mat4::mult_ref(data.of_spotlight.mat_w2sm, inv, data.of_spotlight.mat_tot);
		}
		bool in_shadow(const vec3 &pt, const light::shadow_data &data) const override {
			vec4 cp = data.of_spotlight.mat_tot * vec4(pt);
			vec2 xy;
			cp.homogenize_2(xy);
			data.of_spotlight.buff.denormalize_scr_coord(xy);
			size_t
				x = static_cast<size_t>(clamp(xy.x, 0.5, data.of_spotlight.buff.w - 0.5)),
				y = static_cast<size_t>(clamp(xy.y, 0.5, data.of_spotlight.buff.h - 0.5));
			rtt2_float *z = data.of_spotlight.buff.get_at(x, y, data.of_spotlight.buff.depth_arr), zv = cp.z / cp.w;
			return zv > -1.0 && zv < *z - data.of_spotlight.tolerance;
		}
	};
}
