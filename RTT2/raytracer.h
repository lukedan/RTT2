#pragma once

#include <vector>

#include "vec.h"
#include "mat.h"
#include "utils.h"
#include "model.h"
#include "light.h"

namespace rtt2 {
	namespace raytracing {
		struct camera_info {
			camera_info() = default;
			camera_info(const camera &cam) {
				set(cam);
			}

			void set(const camera &cam) {
				pos = cam.pos;
				rtt2_float horidist = std::tan(0.5 * cam.hori_fov);
				xrange = cam.right_cache * horidist;
				yrange = cam.up * horidist * cam.aspect_ratio;
				origin = cam.forward - xrange - yrange;
				xrange *= 2.0;
				yrange *= 2.0;
			}

			void screen_to_ray(const vec2 &spos, vec3 &ro, vec3 &rd) {
				ro = pos;
				rd = origin + xrange * spos.x + yrange * spos.y;
			}

			vec3 pos, origin, xrange, yrange;
		};

		struct model_cache {
			model_cache() = default;
			model_cache(const rasterizing::model_cache &mc) {
				set(mc);
			}

			void set(const rasterizing::model_cache &mc) {
				pos_cache.resize(mc.pos_cache.size());
				for (size_t i = 0; i < mc.pos_cache.size(); ++i) {
					pos_cache[i] = mc.pos_cache[i].shaded_pos;
				}
				normal_cache.resize(mc.normal_cache.size());
				for (size_t i = 0; i < mc.normal_cache.size(); ++i) {
					normal_cache[i] = mc.normal_cache[i].shaded_normal;
				}
			}

			std::vector<vec3> pos_cache;
			std::vector<vec3> normal_cache;
		};
		struct scene_cache {
			scene_cache() = default;
			scene_cache(const rasterizing::scene_cache &sc) {
				set(sc);
			}

			void set(const rasterizing::scene_cache &sc) {
				of_models.resize(sc.of_models.size());
				for (size_t i = 0; i < of_models.size(); ++i) {
					of_models[i].set(sc.of_models[i]);
				}
			}

			std::vector<model_cache> of_models;
		};

		struct scene_description {
			std::vector<model> models;
			std::vector<light*> lights;
		};

		typedef mem_buffer<color_vec> mem_color_accum_buffer;
		typedef mem_buffer<size_t> mem_hitcount_buffer;
		struct buffer_set {
			buffer_set() = default;
			buffer_set(size_t ww, size_t hh, color_vec *c, size_t *d, device_color *s) : w(ww), h(hh), color_arr(c), stat_arr(d), result_arr(s) {
			}

			size_t w, h;
			color_vec *color_arr;
			size_t *stat_arr;
			device_color *result_arr;

			void set(size_t ww, size_t hh, color_vec *c, size_t *d, device_color *s) {
				w = ww;
				h = hh;
				color_arr = c;
				stat_arr = d;
				result_arr = s;
			}

			void clear() {
				for (size_t i = w * h; i > 0; ) {
					--i;
					color_arr[i] = color_vec();
					stat_arr[i] = 0;
					result_arr[i] = device_color();
				}
			}
			void flush() {
				for (size_t i = w * h; i > 0; ) {
					--i;
					if (stat_arr[i] > 0) {
						color_rgba cv;
						cv.from_vec4(color_arr[i] / stat_arr[i]);
						result_arr[i] = device_color(cv);
					} else {
						result_arr[i] = device_color(0, 0, 0, 0);
					}
				}
			}

			template <typename T> T *get_at(size_t x, size_t y, T *arr) const {
#ifdef DEBUG
				if (x >= w || y >= h) {
					throw std::range_error("invalid coords");
				}
#endif
				return arr + (w * y + x);
			}
		};

		class raytracer {
		public:
			camera_info *cam = nullptr;
			scene_description *scene = nullptr;
			scene_cache *cache = nullptr;
			buffer_set buffer;

			inline static void build_cache_of_model(const model &md, model_cache &mc) {
				mc.pos_cache = std::vector<vec3>(md.data->points.size());
				mc.normal_cache = std::vector<vec3>(md.data->normals.size());
				for (size_t i = 0; i < md.data->points.size(); ++i) {
					transform_default(*md.trans, md.data->points[i], mc.pos_cache[i]);
				}
				for (size_t i = 0; i < md.data->normals.size(); ++i) {
					transform_default(*md.trans, md.data->normals[i], mc.normal_cache[i], 0.0);
				}
			}
			void build_cache(scene_cache &sc) const {
				sc.of_models = std::vector<model_cache>(scene->models.size());
				for (size_t i = 0; i < sc.of_models.size(); ++i) {
					build_cache_of_model(scene->models[i], sc.of_models[i]);
				}
			}
			void build_cache() {
				build_cache(*cache);
			}

			enum class ray_hit_type {
				hit_nothing,
				hit_light,
				hit_model
			};
			struct ray_cast_output {
				ray_hit_type type = ray_hit_type::hit_nothing;
				union {
					struct {
						size_t id;
						size_t face;
						rtt2_float u, v;
					} model;
					light *light;
				} hit;
				vec3 hit_point;
			};
		protected:
			struct _hitpoint_info {
				vec3 normal;
				color_vec_rgb color;
			};
			void _get_hitpoint_info(ray_cast_output &casres, _hitpoint_info &hi) {
				const model &mdl = scene->models[casres.hit.model.id];
				const model_cache &mcc = cache->of_models[casres.hit.model.id];
				const model_data::face_info &fi = mdl.data->faces[casres.hit.model.face];
				hi.normal =
					(1.0 - casres.hit.model.u - casres.hit.model.v) * mcc.normal_cache[fi.normal_ids[0]] +
					casres.hit.model.u * mcc.normal_cache[fi.normal_ids[1]] +
					casres.hit.model.v * mcc.normal_cache[fi.normal_ids[2]];
				hi.normal.set_length(1.0);
				if (mdl.tex) {
					vec2
						p1 = mdl.data->uvs[fi.uv_ids[0]],
						p2 = mdl.data->uvs[fi.uv_ids[1]] - p1,
						p3 = mdl.data->uvs[fi.uv_ids[2]] - p1;
					color_vec cv;
					mdl.tex->sample(
						p1 + p2 * casres.hit.model.u + p3 * casres.hit.model.v,
						cv, uv_clamp_mode::repeat, sample_mode::bilinear
					);
					hi.color = vec_mult(cv.xyz(), mdl.color.xyz());
				} else {
					hi.color = mdl.color.xyz();
				}
			}

			void _ray_cast_impl(
				const vec3 &pos, const vec3 &dir,
				ray_cast_output &output, const ray_cast_output &ignore
			) const {
				output.type = ray_hit_type::hit_nothing;
				rtt2_float min_dist = 0.0;
				hit_test_ray_triangle_results mres;
				for (size_t i = 0; i < scene->models.size(); ++i) {
					model &curmd = scene->models[i];
					const model_cache &curc = cache->of_models[i];
					for (size_t fid = 0; fid < curmd.data->faces.size(); ++fid) {
						if (ignore.type != ray_hit_type::hit_model || ignore.hit.model.id != i || ignore.hit.model.face != fid) {
							const model_data::face_info &fi = curmd.data->faces[fid];
							if (hit_test_ray_triangle(
								pos, dir,
								curc.pos_cache[fi.vertex_ids[0]],
								curc.pos_cache[fi.vertex_ids[1]],
								curc.pos_cache[fi.vertex_ids[2]],
								mres
							)) {
								if (output.type == ray_hit_type::hit_nothing || mres.t < min_dist) {
									min_dist = mres.t;
									output.type = ray_hit_type::hit_model;
									output.hit.model.id = i;
									output.hit.model.u = mres.u;
									output.hit.model.v = mres.v;
									output.hit.model.face = fid;
								}
							}
						}
					}
				}
				light::hit_test_result lres;
				for (auto i = scene->lights.begin(); i != scene->lights.end(); ++i) {
					if (ignore.type != ray_hit_type::hit_light || ignore.hit.light != *i) {
						if ((*i)->hit_test(pos, dir, lres)) {
							if (output.type == ray_hit_type::hit_nothing || lres.t < min_dist) {
								min_dist = lres.t;
								output.type = ray_hit_type::hit_light;
								output.hit.light = *i;
							}
						}
					}
				}
				if (output.type != ray_hit_type::hit_nothing) {
					output.hit_point = pos + dir * min_dist;
				}
			}
		public:
			std::vector<vec3> trace_path_debug(const vec2 &pos, randomizer rand, size_t iters = 5) {
				std::vector<vec3> ret;
				ray_cast_output out, ign;
				vec3 vo, vd;
				color_vec_rgb res(1.0, 1.0, 1.0);
				cam->screen_to_ray(pos, vo, vd);
				vd.set_length(1.0);
				size_t
					x = clamp<size_t>(static_cast<size_t>(std::floor(pos.x * buffer.w)), 0, buffer.w - 1),
					y = clamp<size_t>(static_cast<size_t>(std::floor(pos.y * buffer.h)), 0, buffer.h - 1);
				size_t &statv = *buffer.get_at(x, y, buffer.stat_arr);
				color_vec &colorv = *buffer.get_at(x, y, buffer.color_arr);
				ret.push_back(cam->pos);
				for (size_t i = 0; i < iters; ++i) {
					_ray_cast_impl(vo, vd, out, ign);
					if (out.type == ray_hit_type::hit_model) {
						_hitpoint_info hi;
						_get_hitpoint_info(out, hi);
						vec3 indir = vd;
						vo = out.hit_point;
						vd = get_random_direction_on_hemisphere(hi.normal, rand); // TODO bug maybe
						color_vec_rgb illum;
						scene->models[out.hit.model.id].mtrl.dist_func->get_illum(-vd, -indir, hi.normal, res, res);
						res = vec_mult(res, hi.color);
						ret.push_back(vo);
					} else {
						if (out.type == ray_hit_type::hit_nothing) {
							ret.push_back(vo + vd);
							res = color_vec_rgb();
						} else if (out.type == ray_hit_type::hit_light) {
							ret.push_back(out.hit_point);
							res = vec_mult(res, out.hit.light->illum);
						}
						break;
					}
					ign = out;
				}
				++statv;
				colorv += color_vec(res, 0.0);
				return ret;
			}
			void trace_path(const vec2 &pos, randomizer rand, size_t iters = 5) {
				ray_cast_output out, ign;
				vec3 vo, vd;
				color_vec_rgb res(1.0, 1.0, 1.0);
				cam->screen_to_ray(pos, vo, vd);
				vd.set_length(1.0);
				size_t
					x = clamp<size_t>(static_cast<size_t>(std::floor(pos.x * buffer.w)), 0, buffer.w - 1),
					y = clamp<size_t>(static_cast<size_t>(std::floor(pos.y * buffer.h)), 0, buffer.h - 1);
				size_t &statv = *buffer.get_at(x, y, buffer.stat_arr);
				color_vec &colorv = *buffer.get_at(x, y, buffer.color_arr);
				for (size_t i = 0; i < iters; ++i) {
					_ray_cast_impl(vo, vd, out, ign);
					if (out.type == ray_hit_type::hit_model) {
						_hitpoint_info hi;
						_get_hitpoint_info(out, hi);
						vec3 indir = vd;
						vo = out.hit_point;
						vd = get_random_direction_on_hemisphere(hi.normal, rand); // TODO bug maybe
						color_vec_rgb illum;
						scene->models[out.hit.model.id].mtrl.dist_func->get_illum(-vd, -indir, hi.normal, res, res);
						res = vec_mult(res, hi.color);
					} else {
						if (out.type == ray_hit_type::hit_nothing) {
							res = color_vec_rgb();
						} else if (out.type == ray_hit_type::hit_light) {
							res = vec_mult(res, out.hit.light->illum);
						}
						break;
					}
					ign = out;
				}
				++statv;
				colorv += color_vec(res, 0.0);
			}

			void trace_scene_gist(mem_color_buffer &buf) const {
				ray_cast_output out, ign;
				for (size_t y = 0; y < buf.get_h(); ++y) {
					for (size_t x = 0; x < buf.get_w(); ++x) {
						vec3 vo, vd;
						cam->screen_to_ray(vec2((x + 0.5) / buf.get_w(), (y + 0.5) / buf.get_h()), vo, vd);
						_ray_cast_impl(vo, vd, out, ign);
						color_rgba c(0, 0, 0, 0);
						if (out.type == ray_hit_type::hit_model) {
							const model &mdl = scene->models[out.hit.model.id];
							if (mdl.tex) {
								const model_data::face_info &fi = mdl.data->faces[out.hit.model.face];
								vec2
									p1 = mdl.data->uvs[fi.uv_ids[0]],
									p2 = mdl.data->uvs[fi.uv_ids[1]] - p1,
									p3 = mdl.data->uvs[fi.uv_ids[2]] - p1;
								color_vec cv;
								mdl.tex->sample(
									p1 + p2 * out.hit.model.u + p3 * out.hit.model.v,
									cv, uv_clamp_mode::repeat, sample_mode::bilinear
								);
								c.from_vec4(vec_mult(cv, mdl.color));
							} else {
								c.from_vec4(mdl.color);
							}
						}
						*buf.get_at(x, y) = device_color(c);
					}
				}
			}
		};
	}
}