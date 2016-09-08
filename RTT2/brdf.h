#pragma once

#include "settings.h"
#include "utils.h"
#include "vec.h"
#include "color.h"

namespace rtt2 {
	struct material {
		virtual ~material() {
		}

		virtual void get_illum(const vec3&, const vec3&, const vec3&, const color_vec_rgb&, color_vec_rgb&) const = 0;
	};

	struct material_phong : public material {
		material_phong() = default;
		material_phong(rtt2_float d, rtt2_float sp, rtt2_float sh) : diffuse(d), specular(sp), shiness(sh) {
		}

		rtt2_float diffuse, specular, shiness;

		void get_illum(const vec3 &in, const vec3 &out, const vec3 &normal, const color_vec_rgb &c, color_vec_rgb &res) const override {
			rtt2_float ddotv = vec3::dot(in, normal);
			res = (specular * std::pow(std::max(0.0, vec3::dot(out, in - ddotv * 2.0 * normal)), shiness) - diffuse * ddotv) * c;
		}
	};
}
