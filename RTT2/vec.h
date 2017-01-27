#pragma once

#include <cmath>

#include "settings.h"

#define RTT2_PI 3.1415926535897
#define RTT2_SQRT2 1.4142135623731
#define RTT2_SQRT3 1.7320508075689
#define RTT2_SQRT5 2.2360679774998

namespace rtt2 {
	struct vec2 {
		vec2() = default;
		vec2(rtt2_float px, rtt2_float py) : x(px), y(py) {
		}

		rtt2_float x, y;

		void set_zero() {
			x = y = 0.0;
		}

		rtt2_float length() const {
			return std::sqrt(sqr_length());
		}
		rtt2_float sqr_length() const {
			return x * x + y * y;
		}

		void set_length(rtt2_float len) {
			rtt2_float qt = len / length();
			x *= qt;
			y *= qt;
		}
		void set_length_safe(rtt2_float len, rtt2_float eps = RTT2_EPSILON) {
			rtt2_float qt = length();
			if (qt < eps) {
				x = len;
				y = 0.0;
			} else {
				qt = len / qt;
				x *= qt;
				y *= qt;
			}
		}

		friend vec2 operator +(const vec2 &lhs, const vec2 &rhs) {
			return vec2(lhs.x + rhs.x, lhs.y + rhs.y);
		}
		vec2 &operator +=(const vec2 &rhs) {
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		vec2 operator -() const {
			return vec2(-x, -y);
		}
		friend vec2 operator -(const vec2 &lhs, const vec2 &rhs) {
			return vec2(lhs.x - rhs.x, lhs.y - rhs.y);
		}
		vec2 &operator -=(const vec2 &rhs) {
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		friend vec2 operator *(const vec2 &lhs, rtt2_float rhs) {
			return vec2(lhs.x * rhs, lhs.y * rhs);
		}
		friend vec2 operator *(rtt2_float lhs, const vec2 &rhs) {
			return vec2(lhs * rhs.x, lhs * rhs.y);
		}
		vec2 &operator *=(rtt2_float rhs) {
			x *= rhs;
			y *= rhs;
			return *this;
		}

		friend vec2 operator /(const vec2 &lhs, rtt2_float rhs) {
			return vec2(lhs.x / rhs, lhs.y / rhs);
		}
		vec2 &operator /=(rtt2_float rhs) {
			x /= rhs;
			y /= rhs;
			return *this;
		}

		inline static rtt2_float dot(const vec2 &lhs, const vec2 &rhs) {
			return lhs.x * rhs.x + lhs.y * rhs.y;
		}
		inline static rtt2_float cross(const vec2 &lhs, const vec2 &rhs) {
			return lhs.x * rhs.y - rhs.x * lhs.y;
		}

		inline static vec2 projection(const vec2 &v, const vec2 &axis) {
			return axis * (dot(v, axis) / axis.sqr_length());
		}
	};

	struct vec3 {
		vec3() = default;
		vec3(rtt2_float px, rtt2_float py, rtt2_float pz) : x(px), y(py), z(pz) {
		}
		explicit vec3(const vec2 &v, rtt2_float zz = 1.0) : x(v.x), y(v.y), z(zz) {
		}

		rtt2_float x, y, z;

		void set_zero() {
			x = y = z = 0.0;
		}

		rtt2_float length() const {
			return std::sqrt(sqr_length());
		}
		rtt2_float sqr_length() const {
			return x * x + y * y + z * z;
		}

		void set_length(rtt2_float len) {
			rtt2_float qt = len / length();
			x *= qt;
			y *= qt;
			z *= qt;
		}
		void set_length_safe(rtt2_float len, rtt2_float eps = RTT2_EPSILON) {
			rtt2_float qt = length();
			if (qt < eps) {
				x = len;
				y = z = 0.0;
			} else {
				qt = len / qt;
				x *= qt;
				y *= qt;
				z *= qt;
			}
		}

		friend vec3 operator +(const vec3 &lhs, const vec3 &rhs) {
			return vec3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
		}
		vec3 &operator +=(const vec3 &rhs) {
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}

		vec3 operator -() const {
			return vec3(-x, -y, -z);
		}
		friend vec3 operator -(const vec3 &lhs, const vec3 &rhs) {
			return vec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
		}
		vec3 &operator -=(const vec3 &rhs) {
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}

		friend vec3 operator *(const vec3 &lhs, rtt2_float rhs) {
			return vec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
		}
		friend vec3 operator *(rtt2_float lhs, const vec3 &rhs) {
			return vec3(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
		}
		vec3 &operator *=(rtt2_float rhs) {
			x *= rhs;
			y *= rhs;
			z *= rhs;
			return *this;
		}

		friend vec3 operator /(const vec3 &lhs, rtt2_float rhs) {
			return vec3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
		}
		vec3 &operator /=(rtt2_float rhs) {
			x /= rhs;
			y /= rhs;
			z /= rhs;
			return *this;
		}

		inline static rtt2_float dot(const vec3 &lhs, const vec3 &rhs) {
			return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
		}
		inline static vec3 cross(const vec3 &lhs, const vec3 &rhs) {
			return vec3(lhs.y * rhs.z - rhs.y * lhs.z, lhs.z * rhs.x - rhs.z * lhs.x, lhs.x * rhs.y - rhs.x * lhs.y);
		}
		inline static void cross_ref(const vec3 &lhs, const vec3 &rhs, vec3 &res) {
			res.x = lhs.y * rhs.z - rhs.y * lhs.z;
			res.y = lhs.z * rhs.x - rhs.z * lhs.x;
			res.z = lhs.x * rhs.y - rhs.x * lhs.y;
		}

		inline void get_max_prp(vec3 &res) const {
			vec3 aaa(std::abs(x), std::abs(y), std::abs(z));
			if (aaa.x < aaa.y && aaa.x < aaa.z) {
				cross_ref(*this, vec3(0.0, z, -y), res);
			} else if (aaa.y < aaa.z) {
				cross_ref(*this, vec3(-z, 0.0, x), res);
			} else {
				cross_ref(*this, vec3(y, -x, 0.0), res);
			}
		}

		inline static vec3 projection(const vec3 &v, const vec3 &axis) {
			return axis * (dot(v, axis) / axis.sqr_length());
		}

		vec2 xy() const {
			return vec2(x, y);
		}
		void xy_ref(vec2 &v) const {
			v.x = x;
			v.y = y;
		}
	};

	struct vec4 {
		vec4() = default;
		vec4(rtt2_float px, rtt2_float py, rtt2_float pz, rtt2_float pw) : x(px), y(py), z(pz), w(pw) {
		}
		explicit vec4(const vec3 &v, rtt2_float ww = 1.0) : x(v.x), y(v.y), z(v.z), w(ww) {
		}

		rtt2_float x, y, z, w;

		void set_zero() {
			x = y = z = w = 0.0;
		}

		rtt2_float length() const {
			return std::sqrt(sqr_length());
		}
		rtt2_float sqr_length() const {
			return x * x + y * y + z * z + w * w;
		}

		void set_length(rtt2_float len) {
			rtt2_float qt = len / length();
			x *= qt;
			y *= qt;
			z *= qt;
			w *= qt;
		}
		void set_length_safe(rtt2_float len, rtt2_float eps = RTT2_EPSILON) {
			rtt2_float qt = length();
			if (qt < eps) {
				x = len;
				y = z = w = 0.0;
			} else {
				qt = len / qt;
				x *= qt;
				y *= qt;
				z *= qt;
				w *= qt;
			}
		}

		friend vec4 operator +(const vec4 &lhs, const vec4 &rhs) {
			return vec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
		}
		vec4 &operator +=(const vec4 &rhs) {
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}

		vec4 operator -() const {
			return vec4(-x, -y, -z, -w);
		}
		friend vec4 operator -(const vec4 &lhs, const vec4 &rhs) {
			return vec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
		}
		vec4 &operator -=(const vec4 &rhs) {
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}

		friend vec4 operator *(const vec4 &lhs, rtt2_float rhs) {
			return vec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
		}
		friend vec4 operator *(rtt2_float lhs, const vec4 &rhs) {
			return vec4(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w);
		}
		vec4 &operator *=(rtt2_float rhs) {
			x *= rhs;
			y *= rhs;
			z *= rhs;
			w *= rhs;
			return *this;
		}

		friend vec4 operator /(const vec4 &lhs, rtt2_float rhs) {
			return vec4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
		}
		vec4 &operator /=(rtt2_float rhs) {
			x /= rhs;
			y /= rhs;
			z /= rhs;
			w /= rhs;
			return *this;
		}

		inline static rtt2_float dot(const vec4 &lhs, const vec4 &rhs) {
			return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
		}

		inline static vec4 projection(const vec4 &v, const vec4 &axis) {
			return axis * (dot(v, axis) / axis.sqr_length());
		}

		void homogenize() {
			x /= w;
			y /= w;
			z /= w;
			w = 1.0;
		}
		vec4 homogenized() const {
			return vec4(x / w, y / w, z / w, 1.0);
		}
		void homogenized_ref(vec4 &res) const {
			res.x = x / w;
			res.y = y / w;
			res.z = z / w;
			res.w = 1.0;
		}
		void homogenize_2(vec2 &res) const {
			res.x = x / w;
			res.y = y / w;
		}
		void homogenize_3(vec3 &res) const {
			res.x = x / w;
			res.y = y / w;
			res.z = z / w;
		}

		vec2 xy() const {
			return vec2(x, y);
		}
		void xy_ref(vec2 &v) const {
			v.x = x;
			v.y = y;
		}
		vec3 xyz() const {
			return vec3(x, y, z);
		}
		void xyz_ref(vec3 &v) const {
			v.x = x;
			v.y = y;
			v.z = z;
		}
	};

	inline vec2 vec_mult(const vec2 &v1, const vec2 &v2) {
		return vec2(v1.x * v2.x, v1.y * v2.y);
	}
	inline vec3 vec_mult(const vec3 &v1, const vec3 &v2) {
		return vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
	}
	inline vec4 vec_mult(const vec4 &v1, const vec4 &v2) {
		return vec4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
	}
}
