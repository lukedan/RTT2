#pragma once

#include <atomic>

#include "rasterizer.h"
#include "enhancement.h"

namespace rtt2 {
	struct scene_description {
		std::vector<model> models;
		std::vector<light> lights;
	};

	struct model_cache {
		std::vector<rasterizer::vertex_pos_cache> pos_cache;
		std::vector<rasterizer::vertex_normal_cache> normal_cache;
		dyn_mem_arr enhancement_face_cache;
		mat4 mat_cache;
	};
	struct scene_cache {
		std::vector<model_cache> of_models;
		std::vector<light_cache> of_lights;
		mat4 camview, projection;
	};

	class basic_renderer {
	public:
		rasterizer *linked_rasterizer;
		const scene_description *scene;
		const mat4 *mat_modelview, *mat_projection;
		scene_cache *cache;

		task_lock as_task;
	protected:
		struct additional_shader_info {
			const basic_renderer *r;
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
					r, info, z, st, si->r->cache->of_models[si->modid].enhancement_face_cache.get_at(si->faceid)
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
					r, info, z, st, si->r->cache->of_models[si->modid].enhancement_face_cache.get_at(si->faceid)
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
				if (curl.data->get_illum(info.r->cache->of_lights[i], frag.pos3_cache, in, col)) {
					if (!curl.in_shadow(frag.pos3_cache)) {
						info.r->scene->models[info.modid].mtrl->get_illum(in, npos3, norm, col, cr);
						max_vec(cr, 0.0);
						res += cr;
					}
				}
			}
		}
		inline static void renderer_fragment_shader_compact_withtex(
			const rasterizer&, const rasterizer::frag_info &frag, const texture *tex,
			device_color *cres, void*
		) {
			color_vec c1;
			tex->sample(frag.uv_cache, c1);
			color_vec_rgb rc = c1.xyz() * (0.5 * (
				vec3::dot(vec3(1.0 / RTT2_SQRT3, 1.0 / RTT2_SQRT3, 1.0 / RTT2_SQRT3), frag.normal_cache) +
				vec3::dot(vec3(-0.25, 0.5 * RTT2_SQRT3, 0.0), frag.normal_cache) +
				vec3::dot(vec3(0.0, -0.5735764, 0.819152), frag.normal_cache)
				));
			clamp_vec(rc, 0.0, 1.0);
			cres->from_vec4(vec4(rc, c1.w));
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
		inline static void renderer_fragment_shader_withtex(
			const rasterizer&, const rasterizer::frag_info &frag, const texture *tex,
			device_color *cres, void *pinfo
		) {
			additional_shader_info *info = static_cast<additional_shader_info*>(pinfo);
			color_vec_rgb res(0.0, 0.0, 0.0);
			get_illum_of_frag(frag, *info, res);
			color_vec c1, c2;
			tex->sample(frag.uv_cache, c1);
			color_vec_mult(c1, vec4(res, 1.0), c2);
			color_vec_mult(c2, frag.color_mult_cache, c1);
			clamp_vec(c1, 0.0, 1.0);
			cres->from_vec4(c1);
		}
		inline static void renderer_fragment_shader_notex(
			const rasterizer&, const rasterizer::frag_info &frag, const texture*,
			device_color *cres, void *pinfo
		) {
			additional_shader_info *info = static_cast<additional_shader_info*>(pinfo);
			color_vec_rgb res(0.0, 0.0, 0.0);
			get_illum_of_frag(frag, *info, res);
			color_vec fres;
			color_vec_mult(vec4(res, 1.0), frag.color_mult_cache, fres);
			clamp_vec(fres, 0.0, 1.0);
			cres->from_vec4(fres);
		}

		rasterizer::fragment_shader _fs_tex, _fs_notex;
	public:
		void setup_rendering_env() {
			linked_rasterizer->mat_modelview = &cache->camview;
			linked_rasterizer->mat_proj = &cache->projection;
			linked_rasterizer->shader_vtx = renderer_vertex_shader;
			linked_rasterizer->shader_test = renderer_test_shader;
			_fs_tex = renderer_fragment_shader_withtex;
			_fs_notex = renderer_fragment_shader_notex;
		}
		void setup_rendering_shadow_env() {
			linked_rasterizer->mat_modelview = &cache->camview;
			linked_rasterizer->mat_proj = &cache->projection;
			linked_rasterizer->shader_vtx = renderer_vertex_shader;
			linked_rasterizer->shader_test = renderer_shadow_test_shader;
		}
		void setup_rendering_compact_env() {
			linked_rasterizer->mat_modelview = &cache->camview;
			linked_rasterizer->mat_proj = &cache->projection;
			linked_rasterizer->shader_vtx = renderer_vertex_shader;
			linked_rasterizer->shader_test = rasterizer::default_test_shader;
			_fs_tex = renderer_fragment_shader_compact_withtex;
			_fs_notex = renderer_fragment_shader_compact_notex;
		}
		void render_cached() {
			if (as_task.try_start()) {
				additional_shader_info fi;
				fi.r = this;
				const model *mod = &scene->models[0];
				for (fi.modid = 0; fi.modid < scene->models.size(); ++fi.modid, ++mod) {
					fi.faceid = 0;
					const model_data::face_info *curface = &mod->data->faces[0];
					for (size_t j = 0; j < mod->data->faces.size(); ++j, ++fi.faceid, ++curface) {
						if (as_task.check_cancellation_and_stop()) {
							return;
						}
						if (mod->tex) {
							linked_rasterizer->shader_frag = _fs_tex;
						} else {
							linked_rasterizer->shader_frag = _fs_notex;
						}
						linked_rasterizer->draw_cached_triangle(
							cache->of_models[fi.modid].pos_cache[curface->vertex_ids[0]],
							cache->of_models[fi.modid].pos_cache[curface->vertex_ids[1]],
							cache->of_models[fi.modid].pos_cache[curface->vertex_ids[2]],
							cache->of_models[fi.modid].normal_cache[curface->normal_ids[0]],
							cache->of_models[fi.modid].normal_cache[curface->normal_ids[1]],
							cache->of_models[fi.modid].normal_cache[curface->normal_ids[2]],
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
				as_task.on_stopped();
			}
		}

#pragma region caching
		void init_cache_of_model(const model &m, model_cache &tg) const {
			tg.pos_cache = std::vector<rasterizer::vertex_pos_cache>(m.data->points.size());
			tg.normal_cache = std::vector<rasterizer::vertex_normal_cache>(m.data->normals.size());
			if (m.enhance) {
				tg.enhancement_face_cache.reset(m.enhance->get_additional_face_data_size(), m.data->faces.size());
			}
		}
		void refresh_cache_of_model(const model &m, model_cache &tg) const {
			mat4::mult_ref(cache->camview, *m.trans, tg.mat_cache);
			for (size_t i = 0; i < m.data->points.size(); ++i) {
				transform_default(tg.mat_cache, m.data->points[i], tg.pos_cache[i].shaded_pos);
				tg.pos_cache[i].complete(cache->projection);
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
			t.data->build_cache(cache->camview, c);
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
			sc.camview = *mat_modelview;
			sc.projection = *mat_projection;
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
#pragma endregion
	};
	inline void spotlight::build_shadow_cache(const scene_description &scene, const buffer_set &bs, const shadow_settings &settings, light::shadow_data &sd) const {
		mat4 mdlv, proj;
		vec3 up, right;
		basic_renderer rend;
		rasterizer rast;
		scene_cache cache;

		dir.get_max_prp(up);
		up.set_length(1.0);
		vec3::cross_ref(dir, up, right);
		get_trans_camview_3(pos, dir, up, right, mdlv);
		get_trans_frustrum_3(2.0 * outer_angle, 1.0, settings.of_spotlight.znear, settings.of_spotlight.zfar, proj);

		rast.cur_buf = bs;
		rast.clear_depth_buf(-1.0);

		rend.scene = &scene;
		rend.linked_rasterizer = &rast;
		rend.cache = &cache;
		rend.mat_modelview = &mdlv;
		rend.mat_projection = &proj;

		rend.init_cache();
		rend.refresh_cache();
		rend.setup_rendering_shadow_env();
		rend.render_cached();

		mat4::mult_ref(proj, mdlv, sd.of_spotlight.mat_w2sm);
		sd.of_spotlight.buff = bs;
		sd.of_spotlight.tolerance = settings.of_spotlight.tolerance;
	}
}
