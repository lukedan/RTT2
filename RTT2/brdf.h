#pragma once

#include "settings.h"
#include "utils.h"
#include "vec.h"
#include "color.h"

namespace rtt2 {
	typedef void(*brdf_func)(const vec3&, const vec3&, const vec3&, const color_vec_rgb&, const void*, const color_vec_rgb&);

#define RTT2_DEFINE_BRDF_FUNC(FN)                                                                                                                \
	inline void brdf_##FN(const vec3 &in, const vec3 &out, const vec3 &normal, const color_vec_rgb &c, const void *mat, color_vec_rgb &res) {	 \
		brdf_##FN##_impl(in, out, normal, c, *static_cast<const material_##FN*>(mat), res);                                                      \
	}																																			 \

	struct material_phong {
		material_phong() = default;
		material_phong(rtt2_float d, rtt2_float sp, rtt2_float sh) : diffuse(d), specular(sp), shiness(sh) {
		}

		rtt2_float diffuse, specular, shiness;
	};
	inline void brdf_phong_impl(
		const vec3 &in, const vec3 &out, const vec3 &normal,
		const color_vec_rgb &c, const material_phong &mat, color_vec_rgb &res
	) {
		rtt2_float ddotv = vec3::dot(in, normal);
		res = (mat.specular * std::pow(std::max(0.0, vec3::dot(out, in - ddotv * 2.0 * normal)), mat.shiness) - mat.diffuse * ddotv) * c;
	}
	RTT2_DEFINE_BRDF_FUNC(phong)
}