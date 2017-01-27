#pragma once

#include <Windows.h>
#include <cmath>

#include "vec.h"

namespace rtt2 {
	typedef vec3 color_vec_rgb;
	typedef vec4 color_vec;

	struct color_rgb {
		color_rgb() = default;
		color_rgb(unsigned char rr, unsigned char gg, unsigned char bb) : r(rr), g(gg), b(bb) {
		}

		void from_vec3(const vec3 &v) {
			r = static_cast<unsigned char>(clamp(std::round(v.x * 255.0), 0.0, 255.0));
			g = static_cast<unsigned char>(clamp(std::round(v.y * 255.0), 0.0, 255.0));
			b = static_cast<unsigned char>(clamp(std::round(v.z * 255.0), 0.0, 255.0));
		}
		void to_vec3(vec3 &v) {
			v.x = r / 255.0;
			v.y = g / 255.0;
			v.z = b / 255.0;
		}

		unsigned char r, g, b;
	};
	struct color_rgba {
		color_rgba() = default;
		color_rgba(unsigned char rr, unsigned char gg, unsigned char bb, unsigned char aa) : r(rr), g(gg), b(bb), a(aa) {
		}

		void from_vec4(const vec4 &v) {
			r = static_cast<unsigned char>(clamp(std::round(v.x * 255.0), 0.0, 255.0));
			g = static_cast<unsigned char>(clamp(std::round(v.y * 255.0), 0.0, 255.0));
			b = static_cast<unsigned char>(clamp(std::round(v.z * 255.0), 0.0, 255.0));
			a = static_cast<unsigned char>(clamp(std::round(v.w * 255.0), 0.0, 255.0));
		}
		void to_vec4(vec4 &v) {
			v.x = r / 255.0;
			v.y = g / 255.0;
			v.z = b / 255.0;
			v.w = a / 255.0;
		}

		unsigned char r, g, b, a;
	};

#define RTT2_DEVICE_COLOR_ARGB(A, R, G, B)    \
	(								          \
		(static_cast<DWORD>(A) << 24) |       \
		(static_cast<DWORD>(R) << 16) |       \
		(static_cast<DWORD>(G) << 8) |        \
		static_cast<DWORD>(B)		          \
	)								          \

#define RTT2_DEVICE_COLOR_GETB(X) static_cast<unsigned char>((X) & 0xFF)
#define RTT2_DEVICE_COLOR_GETG(X) static_cast<unsigned char>(((X) >> 8) & 0xFF)
#define RTT2_DEVICE_COLOR_GETR(X) static_cast<unsigned char>(((X) >> 16) & 0xFF)
#define RTT2_DEVICE_COLOR_GETA(X) static_cast<unsigned char>(((X) >> 24) & 0xFF)

#define RTT2_DEVICE_COLOR_SETB(X, B) ((X) ^= ((X) & 0xFF) ^ static_cast<DWORD>(B))
#define RTT2_DEVICE_COLOR_SETG(X, G) ((X) ^= ((X) & 0xFF00) ^ (static_cast<DWORD>(G) << 8))
#define RTT2_DEVICE_COLOR_SETR(X, R) ((X) ^= ((X) & 0xFF0000) ^ (static_cast<DWORD>(R) << 16))
#define RTT2_DEVICE_COLOR_SETA(X, A) ((X) ^= ((X) & 0xFF000000) ^ (static_cast<DWORD>(A) << 24))

	struct device_color {
		device_color() = default;
		device_color(unsigned char a, unsigned char r, unsigned char g, unsigned char b) : argb(RTT2_DEVICE_COLOR_ARGB(a, r, g, b)) {
		}
		explicit device_color(const color_rgba &c) : device_color(c.a, c.r, c.g, c.b) {
		}
		explicit operator color_rgba() const {
			return color_rgba(
				RTT2_DEVICE_COLOR_GETR(argb),
				RTT2_DEVICE_COLOR_GETG(argb),
				RTT2_DEVICE_COLOR_GETB(argb),
				RTT2_DEVICE_COLOR_GETA(argb)
			);
		}

		void from_vec4(const vec4 &v) {
			set(
				static_cast<unsigned char>(v.w * 255.0 + 0.5),
				static_cast<unsigned char>(v.x * 255.0 + 0.5),
				static_cast<unsigned char>(v.y * 255.0 + 0.5),
				static_cast<unsigned char>(v.z * 255.0 + 0.5)
			);
		}
		void to_vec4(vec4 &v) const {
			v.x = RTT2_DEVICE_COLOR_GETR(argb) / 255.0;
			v.y = RTT2_DEVICE_COLOR_GETG(argb) / 255.0;
			v.z = RTT2_DEVICE_COLOR_GETB(argb) / 255.0;
			v.w = RTT2_DEVICE_COLOR_GETA(argb) / 255.0;
		}

		void set(unsigned char a, unsigned char r, unsigned char g, unsigned char b) {
			argb = RTT2_DEVICE_COLOR_ARGB(a, r, g, b);
		}

		unsigned char get_a() const {
			return RTT2_DEVICE_COLOR_GETA(argb);
		}
		unsigned char get_r() const {
			return RTT2_DEVICE_COLOR_GETR(argb);
		}
		unsigned char get_g() const {
			return RTT2_DEVICE_COLOR_GETG(argb);
		}
		unsigned char get_b() const {
			return RTT2_DEVICE_COLOR_GETB(argb);
		}

		void set_a(unsigned char a) {
			RTT2_DEVICE_COLOR_SETA(argb, a);
		}
		void set_r(unsigned char r) {
			RTT2_DEVICE_COLOR_SETR(argb, r);
		}
		void set_g(unsigned char g) {
			RTT2_DEVICE_COLOR_SETG(argb, g);
		}
		void set_b(unsigned char b) {
			RTT2_DEVICE_COLOR_SETB(argb, b);
		}

		DWORD argb;
	};
}