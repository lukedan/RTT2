#pragma once

#include <atomic>

#include "rasterizer.h"
#include "enhancement.h"

namespace rtt2 {
	namespace rasterizing {
		struct scene_description {
			std::vector<model> models;
			std::vector<light> lights;
		};

		struct model_cache {
			std::vector<vertex_pos_cache> pos_cache;
			std::vector<vertex_normal_cache> normal_cache;
			dyn_mem_arr enhancement_face_cache;
			mat4 mat_cache;
		};
		struct scene_cache {
			std::vector<model_cache> of_models;
			std::vector<light_cache> of_lights;
		};

		class basic_renderer {
		public:
			rasterizer *linked_rasterizer = nullptr;
			const scene_description *scene = nullptr;
			const mat4 *mat_modelview = nullptr, *mat_projection = nullptr;
			scene_cache *cache = nullptr;

			struct additional_shader_info {
				additional_shader_info() = default;
				additional_shader_info(const basic_renderer *rend, const scene_cache *cache) : r(rend), sc(cache) {
				}

				const basic_renderer *r = nullptr;
				const scene_cache *sc = nullptr;
				size_t modid, faceid;
			};
			inline static void renderer_vertex_shader(
				const rasterizer&, const mat4 &mat, const vec3 &pos, const vec3 &normal,
				vec3 &rpos, vec3 &rnormal, void*
			) {
				transform_default(mat, pos, rpos);
				transform_default(mat, normal, rnormal, 0.0);
			}
			inline static bool renderer_shadow_test_shader(const rasterizer &r, rasterizer::frag_info &info, rtt2_float *z, unsigned char *st, void *tag) {
				additional_shader_info *si = static_cast<additional_shader_info*>(tag);
				if (si->r->scene->models[si->modid].enhance) {
					if (!si->r->scene->models[si->modid].enhance->before_test_shader(
						r, info, z, st, si->sc->of_models[si->modid].enhancement_face_cache.get_at(si->faceid)
					)) {
						return false;
					}
				}
				if (info.z_cache > *z) {
					*z = info.z_cache;
				}
				return false;
			}
			inline static bool renderer_test_shader(const rasterizer &r, rasterizer::frag_info &info, rtt2_float *z, unsigned char *st, void *tag) {
				additional_shader_info *si = static_cast<additional_shader_info*>(tag);
				if (si->r->scene->models[si->modid].enhance) {
					if (!si->r->scene->models[si->modid].enhance->before_test_shader(
						r, info, z, st, si->sc->of_models[si->modid].enhancement_face_cache.get_at(si->faceid)
					)) {
						return false;
					}
				}
				if (info.z_cache > *z) {
					*z = info.z_cache;
					return true;
				}
				return false;
			}
			inline static void get_illum_of_frag(const rasterizer::frag_info &frag, const additional_shader_info &info, color_vec_rgb &res) {
				vec3 npos3(-frag.pos3_cache), norm(frag.normal_cache);
				npos3.set_length(1.0);
				for (size_t i = 0; i < info.r->scene->lights.size(); ++i) {
					vec3 in;
					color_vec_rgb col, cr;
					const light &curl = info.r->scene->lights[i];
					if (curl.data->get_illum(info.sc->of_lights[i], frag.pos3_cache, in, col)) {
						if (!curl.in_shadow(frag.pos3_cache)) {
							info.r->scene->models[info.modid].mtrl->get_illum(in, npos3, norm, col, cr);
							max_vec(cr, 0.0);
							res += cr;
						}
					}
				}
			}
			inline static void renderer_fragment_shader_compact(
				const rasterizer&, const rasterizer::frag_info &frag, const texture *tex,
				device_color *cres, void*
			) {
				color_vec_rgb c1;
				color_vec c2;
				if (tex) {
					sample(*tex, frag.uv_cache, c2);
				} else {
					c2 = vec4(1.0, 1.0, 1.0, 1.0);
				}
				c1 = c2.xyz() * (0.5 * (
					vec3::dot(vec3(1.0 / RTT2_SQRT3, 1.0 / RTT2_SQRT3, 1.0 / RTT2_SQRT3), frag.normal_cache) +
					vec3::dot(vec3(-0.25, 0.5 * RTT2_SQRT3, 0.0), frag.normal_cache) +
					vec3::dot(vec3(0.0, -0.5735764, 0.819152), frag.normal_cache)
					));
				clamp_vec(c1, 0.0, 1.0);
				cres->from_vec4(vec4(c1, c2.w));
			}
			inline static void renderer_fragment_shader_compact_notex(
				const rasterizer&, const rasterizer::frag_info &frag, const texture*,
				device_color *cres, void*
			) {
				color_vec_rgb c1 = vec3(1.0, 1.0, 1.0) * (0.5 * (
					vec3::dot(vec3(1.0 / RTT2_SQRT3, 1.0 / RTT2_SQRT3, 1.0 / RTT2_SQRT3), frag.normal_cache) +
					vec3::dot(vec3(-0.25, 0.5 * RTT2_SQRT3, 0.0), frag.normal_cache) +
					vec3::dot(vec3(0.0, -0.5735764, 0.819152), frag.normal_cache)
					));
				clamp_vec(c1, 0.0, 1.0);
				cres->from_vec4(vec4(c1, 1.0));
			}
			inline static void renderer_fragment_shader(
				const rasterizer&, const rasterizer::frag_info &frag, const texture *tex,
				device_color *cres, void *pinfo
			) {
				additional_shader_info *info = static_cast<additional_shader_info*>(pinfo);
				color_vec_rgb res(0.0, 0.0, 0.0);
				get_illum_of_frag(frag, *info, res);
				color_vec c1;
				if (tex) {
					sample(*tex, frag.uv_cache, c1);
					c1 = vec_mult(vec_mult(c1, vec4(res, 1.0)), frag.color_mult_cache);
				} else {
					c1 = vec_mult(vec4(res, 1.0), frag.color_mult_cache);
				}
				clamp_vec(c1, 0.0, 1.0);
				cres->from_vec4(c1);
			}
			void setup_custom_rendering_env(
				rasterizer::vertex_shader vs,
				rasterizer::test_shader ts,
				rasterizer::fragment_shader fs
			) {
				linked_rasterizer->mat_modelview = mat_modelview;
				linked_rasterizer->mat_proj = mat_projection;
				linked_rasterizer->shader_vtx = vs;
				linked_rasterizer->shader_test = ts;
				linked_rasterizer->shader_frag = fs;
			}
			void setup_rendering_env() {
				setup_custom_rendering_env(
					renderer_vertex_shader,
					renderer_test_shader,
					renderer_fragment_shader
				);
			}
			void setup_shadow_rendering_env() {
				setup_custom_rendering_env(
					renderer_vertex_shader,
					renderer_shadow_test_shader,
					nullptr
				);
			}
			void setup_compact_rendering_env() {
				setup_custom_rendering_env(
					renderer_vertex_shader,
					rasterizer::default_test_shader,
					renderer_fragment_shader_compact
				);
			}

			void render_cached() {
				render_cached(*cache);
			}
			void render_cached(const scene_cache &sc) {
				additional_shader_info fi(this, &sc);
				const model *mod = &scene->models[0];
				for (fi.modid = 0; fi.modid < scene->models.size(); ++fi.modid, ++mod) {
					fi.faceid = 0;
					const model_data::face_info *curface = &mod->data->faces[0];
					for (size_t j = 0; j < mod->data->faces.size(); ++j, ++fi.faceid, ++curface) {
						linked_rasterizer->draw_cached_triangle(
							sc.of_models[fi.modid].pos_cache[curface->vertex_ids[0]],
							sc.of_models[fi.modid].pos_cache[curface->vertex_ids[1]],
							sc.of_models[fi.modid].pos_cache[curface->vertex_ids[2]],
							sc.of_models[fi.modid].normal_cache[curface->normal_ids[0]],
							sc.of_models[fi.modid].normal_cache[curface->normal_ids[1]],
							sc.of_models[fi.modid].normal_cache[curface->normal_ids[2]],
							mod->data->uvs[curface->uv_ids[0]],
							mod->data->uvs[curface->uv_ids[1]],
							mod->data->uvs[curface->uv_ids[2]],
							mod->color,
							mod->color,
							mod->color,
							mod->tex,
							&fi
						);
					}
				}
			}

			void render_nocache() {
				scene_cache sc;
				init_cache(sc);
				refresh_cache(sc);
				render_cached(sc);
			}

			inline static void init_cache_of_model(const model &m, model_cache &tg) {
				tg.pos_cache = std::vector<vertex_pos_cache>(m.data->points.size());
				tg.normal_cache = std::vector<vertex_normal_cache>(m.data->normals.size());
				if (m.enhance) {
					tg.enhancement_face_cache.reset(m.enhance->get_additional_face_data_size(), m.data->faces.size());
				}
			}
			void refresh_cache_of_model(const model &m, model_cache &tg) const {
				mat4::mult_ref(*mat_modelview, *m.trans, tg.mat_cache);
				for (size_t i = 0; i < m.data->points.size(); ++i) {
					transform_default(tg.mat_cache, m.data->points[i], tg.pos_cache[i].shaded_pos);
					tg.pos_cache[i].complete(*mat_projection);
				}
				for (size_t i = 0; i < m.data->normals.size(); ++i) {
					transform_default(tg.mat_cache, m.data->normals[i], tg.normal_cache[i].shaded_normal, 0.0);
				}
				if (m.enhance) {
					rasterizer::vertex_info vi[3];
					vi[0].c = vi[1].c = vi[2].c = &m.color;
					for (size_t i = 0; i < m.data->faces.size(); ++i) {
						for (size_t d = 0; d < 3; ++d) {
							vi[d].normal = &tg.normal_cache[m.data->faces[i].normal_ids[d]];
							vi[d].pos = &tg.pos_cache[m.data->faces[i].vertex_ids[d]];
							vi[d].uv = &m.data->uvs[m.data->faces[i].uv_ids[d]];
						}
						m.enhance->make_face_cache(vi, tg.enhancement_face_cache.get_at(i));
					}
				}
			}

			void refresh_cache_of_light(const light &t, light_cache &c) const {
				t.data->build_cache(*mat_modelview, c);
			}

			void init_cache(scene_cache &sc) const {
				sc.of_models = std::vector<model_cache>(scene->models.size());
				sc.of_lights = std::vector<light_cache>(scene->lights.size());
				for (size_t i = 0; i < scene->models.size(); ++i) {
					init_cache_of_model(scene->models[i], sc.of_models[i]);
				}
			}
			void init_cache() {
				init_cache(*cache);
			}
			void refresh_cache(scene_cache &sc) const {
				for (size_t i = 0; i < scene->models.size(); ++i) {
					refresh_cache_of_model(scene->models[i], sc.of_models[i]);
				}
				for (size_t i = 0; i < scene->lights.size(); ++i) {
					refresh_cache_of_light(scene->lights[i], sc.of_lights[i]);
				}
			}
			void refresh_cache() {
				refresh_cache(*cache);
			}
		};

		inline void spot_light_data::build_shadow_cache(
			const scene_description &scene,
			const buffer_set *bs,
			const shadow_settings &settings,
			light::shadow_data &sd
		) const {
			mat4 mdlv, proj;
			vec3 up, right;
			basic_renderer rend;
			rasterizer rast;

			dir.get_max_prp(up);
			up.set_length(1.0);
			vec3::cross_ref(dir, up, right);
			get_trans_camview_3(pos, dir, up, right, mdlv);
			get_trans_frustrum_3(2.0 * outer_angle, 1.0, settings.of_spotlight.znear, settings.of_spotlight.zfar, proj);

			rast.cur_buf = *bs;
			rast.clear_depth_buf(-1.0);

			rend.scene = &scene;
			rend.linked_rasterizer = &rast;
			rend.mat_modelview = &mdlv;
			rend.mat_projection = &proj;

			rend.setup_shadow_rendering_env();
			rend.render_nocache();

			sd.of_spotlight.set(mdlv, proj, *bs, settings.of_spotlight.tolerance);
		}

		template <size_t Id> inline void point_light_data::_build_shadow_cache_face(
			const buffer_set *bs,
			basic_renderer &rend,
			mat4 &mdl,
			const mat4 &t1
		) {
			mat4 t2;
			rend.linked_rasterizer->cur_buf = bs[Id];
			rend.linked_rasterizer->clear_depth_buf(-1.0);
			set_rot_mat<Id>(t2);
			mat4::mult_ref(t2, t1, mdl);
			rend.setup_shadow_rendering_env();
			rend.render_nocache();
		}
		inline void point_light_data::build_shadow_cache(
			const scene_description &scene,
			const buffer_set *bs,
			const shadow_settings &settings,
			light::shadow_data &sd
		) const {
			mat4 mdlv, t1;
			basic_renderer rend;
			rasterizer rast;

			get_trans_translation_3(-pos, t1);
			get_trans_frustrum_3(0.5 * RTT2_PI, 1.0, settings.of_pointlight.znear, settings.of_pointlight.zfar, sd.of_pointlight.mat_proj);

			rend.scene = &scene;
			rend.linked_rasterizer = &rast;
			rend.mat_modelview = &mdlv;
			rend.mat_projection = &sd.of_pointlight.mat_proj;

			_build_shadow_cache_face<0>(bs, rend, mdlv, t1);
			_build_shadow_cache_face<1>(bs, rend, mdlv, t1);
			_build_shadow_cache_face<2>(bs, rend, mdlv, t1);
			_build_shadow_cache_face<3>(bs, rend, mdlv, t1);
			_build_shadow_cache_face<4>(bs, rend, mdlv, t1);
			_build_shadow_cache_face<5>(bs, rend, mdlv, t1);

			sd.of_pointlight.set(bs, settings.of_pointlight.tolerance);
		}
	}
}
