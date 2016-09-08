#pragma once

#include "rasterizer.h"

namespace rtt2 {
	struct model_cache {
		std::vector<rasterizer::vertex_pos_cache> pos_cache;
		std::vector<rasterizer::vertex_normal_cache> normal_cache;
		mat4 mat_cache;
	};
	struct scene_cache {
		std::vector<model_cache> of_models;
		std::vector<light_cache> of_lights;
		mat4 camview, perspective;
	};
	class basic_renderer {
	public:
		rasterizer *linked_rasterizer;
		std::vector<model> models;
		std::vector<light> lights;
		camera_spec *cam;

		scene_cache *cache;

	protected:
		inline static void renderer_vertex_shader(
			const rasterizer&, const mat4 &mat, const vec3 &pos, const vec3 &normal,
			vec3 &rpos, vec3 &rnormal, void*
		) {
			transform_default(mat, pos, rpos);
			transform_default(mat, normal, rnormal, 0.0);
		}
		inline static bool renderer_shadow_test_shader(const rasterizer&, const rasterizer::frag_info &info, rtt2_float &z, unsigned char&, void*) {
			if (info.z_cache > z) {
				z = info.z_cache;
			}
			return false;
		}
		inline static bool renderer_test_shader(const rasterizer&, const rasterizer::frag_info &info, rtt2_float &z, unsigned char&, void*) {
			if (info.z_cache > z) {
				z = info.z_cache;
				return true;
			}
			return false;
		}
		struct additional_frag_info {
			const basic_renderer *r;
			size_t modid;
			const model_data::face_info *face;
		};
		inline static void get_illum_of_frag(const rasterizer::frag_info &frag, const additional_frag_info &info, color_vec_rgb &res) {
			vec3 pos3(frag.get_pos3()), npos3(-pos3), norm(frag.get_normal());
			npos3.set_length(1.0);
			for (size_t i = 0; i < info.r->lights.size(); ++i) {
				vec3 in;
				color_vec_rgb col, cr;
				const light &curl = info.r->lights[i];
				if (curl.data->get_illum(info.r->cache->of_lights[i], pos3, in, col)) {
					if (curl.shadow.buff) {
						const model_cache &cac = curl.shadow.scene->of_models[info.modid];
						vec4 cp =
							frag.p * cac.pos_cache[info.face->vertex_ids[frag.v[0].oid]].cam_pos +
							frag.q * cac.pos_cache[info.face->vertex_ids[frag.v[1].oid]].cam_pos +
							frag.r * cac.pos_cache[info.face->vertex_ids[frag.v[2].oid]].cam_pos;
						vec2 xy;
						cp.homogenize_2(xy);
						curl.shadow.buff->denormalize_scr_coord(xy);
						size_t 
							x = static_cast<size_t>(clamp(xy.x, 0.5, curl.shadow.buff->get_w() - 0.5)),
							y = static_cast<size_t>(clamp(xy.y, 0.5, curl.shadow.buff->get_h() - 0.5));
						rtt2_float *z = curl.shadow.buff->get_at(x, y, curl.shadow.buff->get_depth_arr());
						if (cp.z / cp.w < *z - curl.shadow.tolerance) {
							continue;
						}
					}
					info.r->models[info.modid].mtrl->get_illum(in, npos3, norm, col, cr);
					max_vec(cr, 0.0);
					res += cr;
				}
			}
		}
		inline static void renderer_fragment_shader_withtex(
			const rasterizer&, const rasterizer::frag_info &frag, const texture *tex,
			device_color &cres, void *pinfo
		) {
			additional_frag_info *info = static_cast<additional_frag_info*>(pinfo);
			color_vec_rgb res(0.0, 0.0, 0.0);
			get_illum_of_frag(frag, *info, res);
			color_vec c1, c2;
			tex->sample(frag.get_uv(), c1, uv_clamp_mode::repeat, sample_mode::bilinear);
			color_vec_mult(c1, vec4(res, 1.0), c2);
			color_vec_mult(c2, frag.get_color_mult(), c1);
			clamp_vec(c1, 0.0, 1.0);
			cres.from_vec4(c1);
		}
		inline static void renderer_fragment_shader_notex(
			const rasterizer&, const rasterizer::frag_info &frag, const texture*,
			device_color &cres, void *pinfo
		) {
			additional_frag_info *info = static_cast<additional_frag_info*>(pinfo);
			color_vec_rgb res(0.0, 0.0, 0.0);
			get_illum_of_frag(frag, *info, res);
			color_vec fres;
			color_vec_mult(vec4(res, 1.0), frag.get_color_mult(), fres);
			clamp_vec(fres, 0.0, 1.0);
			cres.from_vec4(fres);
		}

	public:
		void setup_rendering_env() const {
			linked_rasterizer->mat_modelview = &cache->camview;
			linked_rasterizer->mat_proj = &cache->perspective;
			linked_rasterizer->shader_vtx = renderer_vertex_shader;
			linked_rasterizer->shader_test = renderer_test_shader;
		}
		void setup_rendering_shadow_env() const {
			linked_rasterizer->mat_modelview = &cache->camview;
			linked_rasterizer->mat_proj = &cache->perspective;
			linked_rasterizer->shader_vtx = renderer_vertex_shader;
			linked_rasterizer->shader_test = renderer_shadow_test_shader;
		}
		void render_cached() const {
			additional_frag_info fi;
			fi.r = this;
			const model *mod = &models[0];
			for (fi.modid = 0; fi.modid < models.size(); ++fi.modid, ++mod) {
				fi.face = &mod->data->faces[0];
				for (size_t j = 0; j < mod->data->faces.size(); ++j, ++fi.face) {
					if (mod->tex) {
						linked_rasterizer->shader_frag = renderer_fragment_shader_withtex;
					} else {
						linked_rasterizer->shader_frag = renderer_fragment_shader_notex;
					}
					linked_rasterizer->draw_cached_triangle(
						cache->of_models[fi.modid].pos_cache[fi.face->vertex_ids[0]],
						cache->of_models[fi.modid].pos_cache[fi.face->vertex_ids[1]],
						cache->of_models[fi.modid].pos_cache[fi.face->vertex_ids[2]],
						cache->of_models[fi.modid].normal_cache[fi.face->normal_ids[0]],
						cache->of_models[fi.modid].normal_cache[fi.face->normal_ids[1]],
						cache->of_models[fi.modid].normal_cache[fi.face->normal_ids[2]],
						mod->data->uvs[fi.face->uv_ids[0]],
						mod->data->uvs[fi.face->uv_ids[1]],
						mod->data->uvs[fi.face->uv_ids[2]],
						mod->color,
						mod->color,
						mod->color,
						mod->tex,
						&fi
					);
				}
			}
		}

#pragma region caching
		void init_cache_of_model(const model &m, model_cache &tg) const {
			tg.pos_cache = std::vector<rasterizer::vertex_pos_cache>(m.data->points.size());
			tg.normal_cache = std::vector<rasterizer::vertex_normal_cache>(m.data->normals.size());
		}
		void refresh_cache_of_model(const model &m, model_cache &tg) const {
			mat4::mult_ref(cache->camview, *m.trans, tg.mat_cache);
			for (size_t i = 0; i != m.data->points.size(); ++i) {
				transform_default(tg.mat_cache, m.data->points[i], tg.pos_cache[i].shaded_pos);
				tg.pos_cache[i].complete(*linked_rasterizer->cur_buf, cache->perspective);
			}
			for (size_t i = 0; i != m.data->normals.size(); ++i) {
				transform_default(tg.mat_cache, m.data->normals[i], tg.normal_cache[i].shaded_normal, 0.0);
			}
		}

		void refresh_cache_of_light(const light_data &t, light_cache &c) const {
			t.build_cache(cache->camview, c);
		}

		void init_cache(scene_cache &sc) const {
			sc.of_models = std::vector<model_cache>(models.size());
			sc.of_lights = std::vector<light_cache>(lights.size());
			for (size_t i = 0; i < models.size(); ++i) {
				init_cache_of_model(models[i], sc.of_models[i]);
			}
		}
		void init_cache() {
			init_cache(*cache);
		}
		void refresh_cache(scene_cache &sc) const {
			get_trans_camview_3(*cam, sc.camview);
			get_trans_frustrum_3(*cam, sc.perspective);
			for (size_t i = 0; i < models.size(); ++i) {
				refresh_cache_of_model(models[i], sc.of_models[i]);
			}
			for (size_t i = 0; i < lights.size(); ++i) {
				refresh_cache_of_light(*lights[i].data, sc.of_lights[i]);
			}
		}
		void refresh_cache() {
			refresh_cache(*cache);
		}
#pragma endregion
	};
}
