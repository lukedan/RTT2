#pragma once

#include "settings.h"
#include "utils.h"
#include "vec.h"
#include "color.h"

namespace rtt2 {
	struct brdf {
		virtual ~brdf() {
		}

		virtual void get_illum(const vec3&, const vec3&, const vec3&, const color_vec_rgb&, color_vec_rgb&) const = 0;
	};

	struct brdf_diffuse : public brdf {
		brdf_diffuse() = default;
		brdf_diffuse(rtt2_float dv) : diffuse(dv) {
		}

		rtt2_float diffuse;

		void get_illum(const vec3 &in, const vec3 &out, const vec3 &normal, const color_vec_rgb &c, color_vec_rgb &res) const override {
			res = -diffuse * vec3::dot(in, normal) * c;
		}
	};
	struct brdf_phong : public brdf {
		brdf_phong() = default;
		brdf_phong(rtt2_float d, rtt2_float sp, rtt2_float sh) : diffuse(d), specular(sp), shiness(sh) {
		}

		rtt2_float diffuse, specular, shiness;

		void get_illum(const vec3 &in, const vec3 &out, const vec3 &normal, const color_vec_rgb &c, color_vec_rgb &res) const override {
			rtt2_float ddotv = vec3::dot(in, normal);
			res = (specular * std::pow(std::max(0.0, vec3::dot(out, in - ddotv * 2.0 * normal)), shiness) - diffuse * ddotv) * c;
		}
	};
	struct brdf_ggx : public brdf { // TODO
		rtt2_float diffuse, specular;

		void get_illum(const vec3 &in, const vec3 &out, const vec3 &normal, const color_vec_rgb &c, color_vec_rgb &res) const override {

		}
	};
}
