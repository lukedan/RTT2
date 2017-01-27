#pragma once

#include <vector>
#include <sstream>

#include "vec.h"
#include "color.h"
#include "utils.h"

namespace rtt2 {
	struct model_data {
	public:
		struct face_info {
			size_t vertex_ids[3], uv_ids[3], normal_ids[3];
		};

		std::vector<vec3> points, normals;
		std::vector<vec2> uvs;
		std::vector<face_info> faces;

		void clear() {
			points.clear();
			normals.clear();
			uvs.clear();
			faces.clear();
		}

		void make_ball(rtt2_float radius) { // TODO use indexed vertices & normals
			points.push_back(vec3(0.0, 0.0, 0.0));
			normals.push_back(vec3(0.0, 0.0, 0.0));
			uvs.push_back(vec2(0.0, 0.0));
			struct tri {
				tri() = default;
				tri(const vec3 &pp1, const vec3 &pp2, const vec3 &pp3) : p1(pp1), p2(pp2), p3(pp3) {
				}

				vec3 p1, p2, p3;
			};
			std::vector<tri> tris;
			// octahedron
			vec3
				top(0.0, 0.0, radius),
				left(0.0, radius, 0.0),
				fore(-radius, 0.0, 0.0),
				right(0.0, -radius, 0.0),
				rear(radius, 0.0, 0.0),
				bottom(0.0, 0.0, -radius);
			tris.push_back(tri(top, left, fore));
			tris.push_back(tri(top, fore, right));
			tris.push_back(tri(top, right, rear));
			tris.push_back(tri(top, rear, left));
			tris.push_back(tri(bottom, fore, left));
			tris.push_back(tri(bottom, left, rear));
			tris.push_back(tri(bottom, rear, right));
			tris.push_back(tri(bottom, right, fore));
			for (size_t i = 0; i < 4; ++i) {
				for (size_t j = tris.size(); j > 0; ) {
					--j;
					tri &curTri = tris[j];
					vec3
						mid12 = (curTri.p1 + curTri.p2),
						mid13 = (curTri.p1 + curTri.p3),
						mid23 = (curTri.p2 + curTri.p3);
					mid12.set_length(radius);
					mid13.set_length(radius);
					mid23.set_length(radius);
					tri lb(mid12, curTri.p2, mid23), rb(mid13, mid23, curTri.p3);
					curTri.p2 = mid12;
					curTri.p3 = mid13;
					tris.push_back(lb);
					tris.push_back(rb);
					tris.push_back(tri(mid12, mid23, mid13));
				}
			}
			for (size_t i = 0; i < tris.size(); ++i) {
				const tri &c = tris[i];
				size_t id = points.size();
				points.push_back(c.p1);
				points.push_back(c.p2);
				points.push_back(c.p3);
				normals.push_back(c.p1 / radius);
				normals.push_back(c.p2 / radius);
				normals.push_back(c.p3 / radius);
				face_info fi;
				for (size_t j = 0; j < 3; ++j) {
					fi.normal_ids[j] = fi.vertex_ids[j] = id + j;
					fi.uv_ids[j] = 0;
				}
				faces.push_back(fi);
			}
		}

		void load_obj(std::ifstream &acc) {
			clear();
			load_obj_noclear(acc);
		}
		void load_obj_noclear(std::ifstream &acc) {
			std::string s1, s2;
			points.push_back(vec3(0.0, 0.0, 0.0));
			normals.push_back(vec3(0.0, 0.0, 0.0));
			uvs.push_back(vec2(0.0, 0.0));
			while (acc) {
				std::getline(acc, s1);
				s2 = "";
				for (size_t i = 0; i < s1.length(); ++i) {
					if (s1[i] == '/') {
						while (s2.length() > 0 && s2.back() == ' ') {
							s2.pop_back();
						}
					}
					s2 += s1[i];
				}
				std::stringstream ss(s2);
				ss >> s1;
				if (s1 == "v") { // vertices
					vec3 p;
					ss >> p.x >> p.y >> p.z;
					points.push_back(p);
				} else if (s1 == "vt") { // uvs
					vec2 p;
					ss >> p.x >> p.y;
					uvs.push_back(p);
				} else if (s1 == "vn") { // normals
					vec3 p;
					ss >> p.x >> p.y >> p.z;
					normals.push_back(p);
				} else if (s1 == "f") { // faces
					std::string obj;
					face_info info;
					for (size_t i = 0; i < 3; ++i) {
						ss >> obj;
						const char *last = obj.c_str();
						size_t *vp = &info.vertex_ids[i], curc = 0;
						for (size_t p = 0; p < 3; ++p, vp += 3, ++curc) {
							if (curc >= obj.length()) {
								*vp = 0;
							} else {
								last = &obj[curc];
								for (; curc < obj.length(); ++curc) {
									if (obj[curc] == '/') {
										break;
									}
								}
								if (last == &obj[curc]) {
									*vp = 0;
								} else {
									*vp = std::atoi(last);
								}
							}
						}
					}
					faces.push_back(info);
				}
			}
		}

		void generate_normals_weighted_average() {
			std::vector<vec3> totc(points.size(), vec3(0.0, 0.0, 0.0));
			size_t id = 1;
			for (auto i = faces.begin(); i != faces.end(); ++i, ++id) {
				vec3 x = vec3::cross(points[i->vertex_ids[1]] - points[i->vertex_ids[0]], points[i->vertex_ids[2]] - points[i->vertex_ids[0]]);
				for (size_t p = 0; p < 3; ++p) {
					size_t cid = i->vertex_ids[p];
					totc[cid - 1] += x;
					i->normal_ids[p] = cid;
				}
			}
			for (size_t i = 0; i < totc.size(); ++i) {
				vec3 v = totc[i];
				v.set_length(1.0);
				normals.push_back(v);
			}
		}
		void generate_normals_flat() {
			size_t id = 1;
			for (auto i = faces.begin(); i != faces.end(); ++i, ++id) {
				normals.push_back(vec3::cross(points[i->vertex_ids[1]] - points[i->vertex_ids[0]], points[i->vertex_ids[2]] - points[i->vertex_ids[0]]));
				for (size_t p = 0; p < 3; ++p) {
					i->normal_ids[p] = id;
				}
			}
		}
	};

	namespace rasterizing {
		class enhancement;
		struct model {
			model() = default;
			model(
				const model_data *d,
				const brdf *m,
				const mat4 *t,
				const texture *te,
				const enhancement *enh,
				const color_vec &c
			) : data(d), mtrl(m), trans(t), tex(te), enhance(enh), color(c) {
			}

			const model_data *data = nullptr;
			const brdf *mtrl = nullptr;
			const mat4 *trans = nullptr;
			const texture *tex = nullptr;
			const enhancement *enhance = nullptr;
			color_vec color;
		};
	}
	namespace raytracing {
		struct material {
			material() = default;
			material(const brdf *func) : dist_func(func) {
			}

			const brdf *dist_func = nullptr;
		};
		struct model {
			model() = default;
			model(
				const model_data *dt,
				const mat4 *tr,
				const texture *t,
				const material &mt,
				const color_vec &c
			) : data(dt), trans(tr), tex(t), mtrl(mt), color(c) {
			}

			const model_data *data = nullptr;
			const mat4 *trans = nullptr;
			const texture *tex = nullptr;
			material mtrl;
			color_vec color;
		};
	}
}