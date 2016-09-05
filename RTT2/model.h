#pragma once

#include <vector>
#include <sstream>

#include "vec.h"
#include "color.h"
#include "utils.h"

namespace rtt2 {
	struct model {
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

		void load_obj(file_access &acc) {
			clear();
			load_obj_noclear(acc);
		}
		void load_obj_noclear(file_access &acc) {
			std::string s1, s2;
			points.push_back(vec3(0.0, 0.0, 0.0));
			normals.push_back(vec3(0.0, 0.0, 0.0));
			uvs.push_back(vec2(0.0, 0.0));
			while (acc) {
				acc.get_line(s1);
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
						size_t *vp = &info.vertex_ids[i];
						size_t curc = 0;
						for (size_t p = 0; p < 3; ++p, vp += 3, ++curc) {
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
					faces.push_back(info);
				}
			}
		}
	};
}