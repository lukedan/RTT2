#pragma once

#include "vec.h"
#include "color.h"

namespace rtt2 {
	enum class light_type {
		pointlight,
		directional,
		spotlight
	};
	struct light {
		light_type type;
		union {
			struct {
				vec3 pos;
				color_vec_rgb color;
				rtt2_float strength;
			} of_pointlight;

			struct {
				vec3 dir;
				color_vec_rgb color;
			} of_directional;

			struct {
				vec3 pos, dir;
				color_vec_rgb color;
				rtt2_float angle;
			} of_spotlight;
		} data;
	};
}