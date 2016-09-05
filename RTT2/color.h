#pragma once

#include <Windows.h>
#include <cmath>

#include "vec.h"

namespace rtt2 {
	typedef vec3 color_vec_rgb;
	typedef vec4 color_vec;

	void color_vec_mult(const color_vec_rgb &a, const color_vec_rgb &b, color_vec &r) {
		r.x = a.x * b.x;
		r.y = a.y * b.y;
		r.z = a.z * b.z;
	}
	void color_vec_mult(const color_vec &a, const color_vec &b, color_vec &r) {
		r.x = a.x * b.x;
		r.y = a.y * b.y;
		r.z = a.z * b.z;
		r.w = a.w * b.w;
	}

	struct color_rgb {
		color_rgb() = default;
		color_rgb(unsigned char rr, unsigned char gg, unsigned char bb) : r(rr), g(gg), b(bb) {
		}

		void from_vec3(const vec3 &v) {
			r = static_cast<unsigned char>(std::round(v.x * 255.0));
			g = static_cast<unsigned char>(std::round(v.y * 255.0));
			b = static_cast<unsigned char>(std::round(v.z * 255.0));
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
			r = static_cast<unsigned char>(std::round(v.x * 255.0));
			g = static_cast<unsigned char>(std::round(v.y * 255.0));
			b = static_cast<unsigned char>(std::round(v.z * 255.0));
			a = static_cast<unsigned char>(std::round(v.w * 255.0));
		}
		void to_vec4(vec4 &v) {
			v.x = r / 255.0;
			v.y = g / 255.0;
			v.z = b / 255.0;
			v.w = a / 255.0;
		}

		void mult(const color_rgba &rhs) {
			r = static_cast<unsigned char>(std::round((static_cast<int>(r) * rhs.r) / 255.0));
			g = static_cast<unsigned char>(std::round((static_cast<int>(g) * rhs.g) / 255.0));
			b = static_cast<unsigned char>(std::round((static_cast<int>(b) * rhs.b) / 255.0));
			a = static_cast<unsigned char>(std::round((static_cast<int>(a) * rhs.a) / 255.0));
		}
		void mix(const color_rgba &c1, const color_rgba &c2, const color_rgba &c3, rtt2_float v1, rtt2_float v2, rtt2_float v3) {
			r = static_cast<unsigned char>(std::round(c1.r * v1 + c2.r * v2 + c3.r * v3));
			g = static_cast<unsigned char>(std::round(c1.g * v1 + c2.g * v2 + c3.g * v3));
			b = static_cast<unsigned char>(std::round(c1.b * v1 + c2.b * v2 + c3.b * v3));
			a = static_cast<unsigned char>(std::round(c1.a * v1 + c2.a * v2 + c3.a * v3));
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

		// two funcs below seem to be extra slow
		void mult(const device_color &rhs) {
			set(
				static_cast<unsigned char>(std::round((static_cast<int>(get_a()) * rhs.get_a()) / 255.0)),
				static_cast<unsigned char>(std::round((static_cast<int>(get_r()) * rhs.get_r()) / 255.0)),
				static_cast<unsigned char>(std::round((static_cast<int>(get_g()) * rhs.get_g()) / 255.0)),
				static_cast<unsigned char>(std::round((static_cast<int>(get_b()) * rhs.get_b()) / 255.0))
			);
		}
		void mix(const device_color &c1, const device_color &c2, const device_color &c3, rtt2_float v1, rtt2_float v2, rtt2_float v3) {
			set(
				static_cast<unsigned char>(std::round(c1.get_a() * v1 + c2.get_a() * v2 + c3.get_a() * v3)),
				static_cast<unsigned char>(std::round(c1.get_r() * v1 + c2.get_r() * v2 + c3.get_r() * v3)),
				static_cast<unsigned char>(std::round(c1.get_g() * v1 + c2.get_g() * v2 + c3.get_g() * v3)),
				static_cast<unsigned char>(std::round(c1.get_b() * v1 + c2.get_b() * v2 + c3.get_b() * v3))
			);
		}

		DWORD argb;
	};
}