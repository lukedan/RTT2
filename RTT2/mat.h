#pragma once

#include <utility>
#include <cstring>
#include <exception>

#include "vec.h"
#include "utils.h"

namespace rtt2 {
	template <size_t Dim> struct sqrmat {
		typedef rtt2_float column[Dim];

		column val[Dim];

		column &operator [](size_t x) {
			return val[x];
		}
		const column &operator [](size_t x) const {
			return val[x];
		}

		void transpose_in_place() {
			for (size_t y = 1; y < Dim; ++y) {
				for (size_t x = 0; x < y; ++x) {
					std::swap(val[x][y], val[y][x]);
				}
			}
		}
		void get_transposition(sqrmat &res) const {
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					res[x][y] = val[y][x];
				}
			}
		}

		inline static rtt2_float get_det_2(rtt2_float m11, rtt2_float m12, rtt2_float m21, rtt2_float m22) {
			return m11 * m22 - m12 * m21;
		}
		inline static rtt2_float get_det_3(
			rtt2_float m11, rtt2_float m12, rtt2_float m13,
			rtt2_float m21, rtt2_float m22, rtt2_float m23,
			rtt2_float m31, rtt2_float m32, rtt2_float m33
		) {
			return
				m11 * get_det_2(m22, m23, m32, m33) -
				m12 * get_det_2(m21, m23, m31, m33) +
				m13 * get_det_2(m21, m22, m31, m32);
		}
		inline static rtt2_float get_det_4(
			rtt2_float m11, rtt2_float m12, rtt2_float m13, rtt2_float m14,
			rtt2_float m21, rtt2_float m22, rtt2_float m23, rtt2_float m24,
			rtt2_float m31, rtt2_float m32, rtt2_float m33, rtt2_float m34,
			rtt2_float m41, rtt2_float m42, rtt2_float m43, rtt2_float m44
		) {
			return
				m11 * get_det_3(m22, m23, m24, m32, m33, m34, m42, m43, m44) -
				m12 * get_det_3(m21, m23, m24, m31, m33, m34, m41, m43, m44) +
				m13 * get_det_3(m21, m22, m24, m31, m32, m34, m41, m42, m44) -
				m14 * get_det_3(m21, m22, m23, m31, m32, m33, m41, m42, m43);
		}

		inline static void get_adjugate_mat_2(rtt2_float m11, rtt2_float m12, rtt2_float m21, rtt2_float m22, sqrmat<2> &res) {
			res[0][0] = m22;
			res[0][1] = -m21;
			res[1][0] = -m12;
			res[1][1] = m11;
		}
		inline static void get_adjugate_mat_3(
			rtt2_float m11, rtt2_float m12, rtt2_float m13,
			rtt2_float m21, rtt2_float m22, rtt2_float m23,
			rtt2_float m31, rtt2_float m32, rtt2_float m33,
			sqrmat<3> &res
		) {
			res[0][0] = get_det_2(m22, m23, m32, m33);
			res[0][1] = -get_det_2(m21, m23, m31, m33);
			res[0][2] = get_det_2(m21, m22, m31, m32);

			res[1][0] = -get_det_2(m12, m13, m32, m33);
			res[1][1] = get_det_2(m11, m13, m31, m33);
			res[1][2] = -get_det_2(m11, m12, m31, m32);

			res[2][0] = get_det_2(m12, m13, m22, m23);
			res[2][1] = -get_det_2(m11, m13, m21, m23);
			res[2][2] = get_det_2(m11, m12, m21, m22);
		}
		inline static void get_adjugate_mat_4(
			rtt2_float m11, rtt2_float m12, rtt2_float m13, rtt2_float m14,
			rtt2_float m21, rtt2_float m22, rtt2_float m23, rtt2_float m24,
			rtt2_float m31, rtt2_float m32, rtt2_float m33, rtt2_float m34,
			rtt2_float m41, rtt2_float m42, rtt2_float m43, rtt2_float m44,
			sqrmat<4> &res
		) {
			res[0][0] = get_det_3(m22, m23, m24, m32, m33, m34, m42, m43, m44);
			res[0][1] = -get_det_3(m21, m23, m24, m31, m33, m34, m41, m43, m44);
			res[0][2] = get_det_3(m21, m22, m24, m31, m32, m34, m41, m42, m44);
			res[0][3] = -get_det_3(m21, m22, m23, m31, m32, m33, m41, m42, m43);

			res[1][0] = -get_det_3(m12, m13, m14, m32, m33, m34, m42, m43, m44);
			res[1][1] = get_det_3(m11, m13, m14, m31, m33, m34, m41, m43, m44);
			res[1][2] = -get_det_3(m11, m12, m14, m31, m32, m34, m41, m42, m44);
			res[1][3] = get_det_3(m11, m12, m13, m31, m32, m33, m41, m42, m43);

			res[2][0] = get_det_3(m12, m13, m14, m22, m23, m24, m42, m43, m44);
			res[2][1] = -get_det_3(m11, m13, m14, m21, m23, m24, m41, m43, m44);
			res[2][2] = get_det_3(m11, m12, m14, m21, m22, m24, m41, m42, m44);
			res[2][3] = -get_det_3(m11, m12, m13, m21, m22, m23, m41, m42, m43);

			res[3][0] = -get_det_3(m12, m13, m14, m22, m23, m24, m32, m33, m34);
			res[3][1] = get_det_3(m11, m13, m14, m21, m23, m24, m31, m33, m34);
			res[3][2] = -get_det_3(m11, m12, m14, m21, m22, m24, m31, m32, m34);
			res[3][3] = get_det_3(m11, m12, m13, m21, m22, m23, m31, m32, m33);
		}

#define RTT2_MAT_SERIALIZE2(N) (N)[0][0], (N)[1][0], (N)[0][1], (N)[1][1]
#define RTT2_MAT_SERIALIZE3(N)          \
	(N)[0][0], (N)[1][0], (N)[2][0],    \
	(N)[0][1], (N)[1][1], (N)[2][1],    \
	(N)[0][2], (N)[1][2], (N)[2][2]     \

#define RTT2_MAT_SERIALIZE4(N)                     \
	val[0][0], val[1][0], val[2][0], val[3][0],    \
	val[0][1], val[1][1], val[2][1], val[3][1],    \
	val[0][2], val[1][2], val[2][2], val[3][2],    \
	val[0][3], val[1][3], val[2][3], val[3][3]     \

		template <size_t D> struct helper {
		};
		rtt2_float get_determination_impl(const helper<2>&) const {
			return get_det_2(RTT2_MAT_SERIALIZE2(val));
		}
		rtt2_float get_determination_impl(const helper<3>&) const {
			return get_det_3(RTT2_MAT_SERIALIZE3(val));
		}
		rtt2_float get_determination_impl(const helper<4>&) const {
			return get_det_4(RTT2_MAT_SERIALIZE4(val));
		}
		rtt2_float get_determination() const {
			return get_determination_impl(helper<Dim>());
		}

		void get_inversion_gaussian_unstable(sqrmat &res) const {
			sqrmat tmp(*this);
			res.set_identity();
			for (size_t x = 0; x < Dim; ++x) {
				for (size_t y = 0; y < Dim; ++y) {
					if (x == y) {
						continue;
					}
					rtt2_float ratio = tmp[x][y] / tmp[x][x];
					for (size_t i = x, j = 0; i < Dim; ++i, ++j) {
						tmp[i][y] -= tmp[i][x] * ratio;
						res[j][y] -= res[j][x] * ratio;
					}
				}
			}
		}
		void get_inversion_adj_impl(sqrmat &res, const helper<2>&) const {
			get_adjugate_mat_2(RTT2_MAT_SERIALIZE2(val), res);
			res /= get_det_2(RTT2_MAT_SERIALIZE2(val));
		}
		void get_inversion_adj_impl(sqrmat &res, const helper<3>&) const {
			get_adjugate_mat_3(RTT2_MAT_SERIALIZE3(val), res);
			res /= get_det_3(RTT2_MAT_SERIALIZE3(val));
		}
		void get_inversion_adj_impl(sqrmat &res, const helper<4>&) const {
			get_adjugate_mat_4(RTT2_MAT_SERIALIZE4(val), res);
			res /= get_det_4(RTT2_MAT_SERIALIZE4(val));
		}
		void get_inversion_adj(sqrmat &res) const {
			return get_inversion_adj_impl(res, helper<Dim>());
		}

		void get_inversion_impl(sqrmat &res, const helper<2>&) const {
			get_inversion_adj(res);
		}
		void get_inversion_impl(sqrmat &res, const helper<3>&) const {
			get_inversion_adj(res);
		}
		void get_inversion_impl(sqrmat &res, const helper<4>&) const {
			get_inversion_adj(res);
		}
		void get_inversion(sqrmat &res) const {
			get_inversion_impl(res, helper<Dim>());
		}

		void set_identity() {
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					val[x][y] = (x == y ? 1.0 : 0.0);
				}
			}
		}
		void set_zero() {
			std::memset(val, 0, sizeof(val));
		}

		friend sqrmat operator +(const sqrmat &lhs, const sqrmat &rhs) {
			sqrmat res;
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					res[x][y] = lhs[x][y] + rhs[x][y];
				}
			}
			return res;
		}
		sqrmat &operator +=(const sqrmat &rhs) {
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					val[x][y] += rhs[x][y];
				}
			}
			return *this;
		}

		friend sqrmat operator *(const sqrmat &lhs, rtt2_float rhs) {
			sqrmat res;
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					res[x][y] = lhs[x][y] * rhs;
				}
			}
			return res;
		}
		friend sqrmat operator *(rtt2_float lhs, const sqrmat &rhs) {
			return rhs * lhs;
		}
		sqrmat &operator *=(rtt2_float rhs) {
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					val[x][y] *= rhs;
				}
			}
			return *this;
		}

		inline static void mult_ref(const sqrmat &lhs, const sqrmat &rhs, sqrmat &res) {
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					res[x][y] = 0.0;
					for (size_t i = 0; i < Dim; ++i) {
						res[x][y] += lhs[i][y] * rhs[x][i];
					}
				}
			}
		}
		friend sqrmat operator *(const sqrmat &lhs, const sqrmat &rhs) {
			sqrmat res;
			mult_ref(lhs, rhs, res);
			return res;
		}
		sqrmat &operator *=(const sqrmat &rhs) {
			return ((*this) = (*this) * rhs);
		}

		friend sqrmat operator /(const sqrmat &lhs, rtt2_float rhs) {
			sqrmat res;
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					res[x][y] = lhs[x][y] / rhs;
				}
			}
			return res;
		}
		sqrmat &operator /=(rtt2_float rhs) {
			for (size_t y = 0; y < Dim; ++y) {
				for (size_t x = 0; x < Dim; ++x) {
					val[x][y] /= rhs;
				}
			}
			return *this;
		}
	};
	typedef sqrmat<2> mat2;
	typedef sqrmat<3> mat3;
	typedef sqrmat<4> mat4;

	inline vec2 operator *(const mat2 &lhs, const vec2 &rhs) {
		return vec2(lhs[0][0] * rhs.x + lhs[1][0] * rhs.y, lhs[0][1] * rhs.x + lhs[1][1] * rhs.y);
	}
	inline vec3 operator *(const mat3 &lhs, const vec3 &rhs) {
		return vec3(
			lhs[0][0] * rhs.x + lhs[1][0] * rhs.y + lhs[2][0] * rhs.z,
			lhs[0][1] * rhs.x + lhs[1][1] * rhs.y + lhs[2][1] * rhs.z,
			lhs[0][2] * rhs.x + lhs[1][2] * rhs.y + lhs[2][2] * rhs.z
		);
	}
	inline vec4 operator *(const mat4 &lhs, const vec4 &rhs) {
		return vec4(
			lhs[0][0] * rhs.x + lhs[1][0] * rhs.y + lhs[2][0] * rhs.z + lhs[3][0] * rhs.w,
			lhs[0][1] * rhs.x + lhs[1][1] * rhs.y + lhs[2][1] * rhs.z + lhs[3][1] * rhs.w,
			lhs[0][2] * rhs.x + lhs[1][2] * rhs.y + lhs[2][2] * rhs.z + lhs[3][2] * rhs.w,
			lhs[0][3] * rhs.x + lhs[1][3] * rhs.y + lhs[2][3] * rhs.z + lhs[3][3] * rhs.w
		);
	}

	inline void transform_default(const mat3 &mt, const vec2 &v, vec2 &res, rtt2_float z = 1.0) {
		res.x = mt[0][0] * v.x + mt[1][0] * v.y + mt[2][0] * z;
		res.y = mt[0][1] * v.x + mt[1][1] * v.y + mt[2][1] * z;
	}
	inline void transform_default(const mat4 &mt, const vec3 &v, vec3 &res, rtt2_float w = 1.0) {
		res.x = mt[0][0] * v.x + mt[1][0] * v.y + mt[2][0] * v.z + mt[3][0] * w;
		res.y = mt[0][1] * v.x + mt[1][1] * v.y + mt[2][1] * v.z + mt[3][1] * w;
		res.z = mt[0][2] * v.x + mt[1][2] * v.y + mt[2][2] * v.z + mt[3][2] * w;
	}

	inline void make_mat2_from_vec2s(const vec2 &c1, const vec2 &c2, mat2 &mat) {
		std::memcpy(mat[0], &c1, sizeof(vec2));
		std::memcpy(mat[1], &c2, sizeof(vec2));
	}
	inline void make_mat3_from_vec2s(const vec2 &c1, const vec2 &c2, const vec2 &c3, mat3 &mat) {
		std::memcpy(mat[0], &c1, sizeof(vec2));
		std::memcpy(mat[1], &c2, sizeof(vec2));
		std::memcpy(mat[2], &c3, sizeof(vec2));
		mat[0][2] = mat[1][2] = mat[2][2] = 1.0;
	}
	inline void make_mat3_from_vec3s(const vec3 &c1, const vec3 &c2, const vec3 &c3, mat3 &mat) {
		std::memcpy(mat[0], &c1, sizeof(vec3));
		std::memcpy(mat[1], &c2, sizeof(vec3));
		std::memcpy(mat[2], &c3, sizeof(vec3));
	}
	inline void make_mat4_from_vec3s(const vec3 &c1, const vec3 &c2, const vec3 &c3, const vec3 &c4, mat4 &mat) {
		std::memcpy(mat[0], &c1, sizeof(vec3));
		std::memcpy(mat[1], &c2, sizeof(vec3));
		std::memcpy(mat[2], &c3, sizeof(vec3));
		std::memcpy(mat[3], &c4, sizeof(vec3));
		mat[0][3] = mat[1][3] = mat[2][3] = mat[3][3] = 1.0;
	}
	inline void make_mat4_from_vec4s(const vec4 &c1, const vec4 &c2, const vec4 &c3, const vec4 &c4, mat4 &mat) {
		std::memcpy(mat[0], &c1, sizeof(vec4));
		std::memcpy(mat[1], &c2, sizeof(vec4));
		std::memcpy(mat[2], &c3, sizeof(vec4));
		std::memcpy(mat[3], &c4, sizeof(vec4));
	}

	inline void get_trans_pts_to_pts_2(
		const vec2 &pf1, const vec2 &pf2, const vec2 &pf3,
		const vec2 &pt1, const vec2 &pt2, const vec2 &pt3,
		mat3 &res
	) {
		mat3 t1, t2;
		make_mat3_from_vec2s(pf1, pf2, pf3, res);
		res.get_inversion(t1);
		make_mat3_from_vec2s(pt1, pt2, pt3, t2);
		mat3::mult_ref(t2, t1, res);
	}
	inline void get_trans_rotate_2(const vec2 &center, rtt2_float angle, mat3 &res) {
		rtt2_float cosv = std::cos(angle), sinv = std::sin(angle), omcv = 1.0 - cosv;
		res[0][0] = res[1][1] = cosv;
		res[1][0] = -(res[0][1] = sinv);
		res[2][0] = center.x * omcv + center.y * sinv;
		res[2][1] = center.y * omcv - center.x * sinv;
		res[0][2] = res[1][2] = 0.0;
		res[2][2] = 1.0;
	}
	inline void get_trans_rotate_2_met2(const vec2 &center, rtt2_float angle, mat3 &res) {
		rtt2_float cosv = std::cos(angle), sinv = std::sin(angle);
		get_trans_pts_to_pts_2(
			center, vec2(center.x + 1.0, center.y), vec2(center.x, center.y + 1.0),
			center, vec2(center.x + cosv, center.y + sinv), vec2(center.x - sinv, center.y + cosv),
			res
		);
	}
	inline void get_trans_rotate_2_met3(const vec2 &center, rtt2_float angle, mat3 &res) { // seems to be slower
		rtt2_float cosv = std::cos(angle), sinv = std::sin(angle);
		res[0][0] = res[1][1] = cosv;
		res[1][0] = -(res[0][1] = sinv);
		res[0][2] = res[1][2] = res[2][0] = res[2][1] = 0.0;
		res[2][2] = 1.0;

		mat3 t1, t2;
		t1[0][0] = t1[1][1] = t1[2][2] = 1.0;
		t1[0][1] = t1[0][2] = t1[1][0] = t1[1][2] = 0.0;
		t1[2][0] = center.x;
		t1[2][1] = center.y;
		mat3::mult_ref(t1, res, t2);
		res = t1 * res;

		t1[2][0] = -center.x;
		t1[2][1] = -center.y;
		mat3::mult_ref(t2, t1, res);
	}
	inline void get_trans_translation_2(const vec2 &diff, mat3 &res) {
		res[0][0] = res[1][1] = res[2][2] = 1.0;
		res[0][1] = res[0][2] = res[1][0] = res[1][2] = 0.0;
		res[2][0] = diff.x;
		res[2][1] = diff.y;
	}
	inline void get_trans_screenspace_2(const vec2 &size, mat3 &res) {
		res[0][0] = res[2][0] = 0.5 * size.x;
		res[1][1] = res[2][1] = 0.5 * size.y;
		res[2][2] = 1.0;
		res[0][1] = res[0][2] = res[1][0] = res[1][2] = 0.0;
	}

	struct camera_spec {
		vec3 pos, forward, up, right_cache;
		rtt2_float aspect_ratio, hori_fov, znear, zfar;

		void make_cache() {
			vec3::cross_ref(forward, up, right_cache);
		}
	};

	inline void get_trans_pts_to_pts_3(
		const vec3 &pf1, const vec3 &pf2, const vec3 &pf3, const vec3 &pf4,
		const vec3 &pt1, const vec3 &pt2, const vec3 &pt3, const vec3 &pt4,
		mat4 &res
	) {
		mat4 t1, t2;
		make_mat4_from_vec3s(pf1, pf2, pf3, pf4, res);
		res.get_inversion(t1);
		make_mat4_from_vec3s(pt1, pt2, pt3, pt4, t2);
		mat4::mult_ref(t2, t1, res);
	}
	inline void get_trans_translation_3(const vec3 &diff, mat4 &res) {
		res[0][0] = res[1][1] = res[2][2] = res[3][3] = 1.0;
		res[0][1] = res[0][2] = res[0][3] = res[1][0] = res[1][2] = res[1][3] = res[2][0] = res[2][1] = res[2][3] = 0.0;
		res[3][0] = diff.x;
		res[3][1] = diff.y;
		res[3][2] = diff.z;
	}
	inline void get_trans_rotation_3(const vec3 &center, const vec3 &axis, rtt2_float angle, mat4 &res) {
		vec3 x, y;
		axis.get_max_prp(x);
		vec3::cross_ref(axis, x, y);
		x.set_length(y.length());
		rtt2_float cosv = std::cos(angle), sinv = std::sin(angle);
		get_trans_pts_to_pts_3(center, center + axis, center + x, center + y, center, center + axis, center + cosv * x + sinv * y, center - sinv * x + cosv * y, res);
		// TODO see if this is right
	}
	inline void get_trans_camview_3(const vec3 &pos, const vec3 &dir, const vec3 &up, const vec3 &right, mat4 &res) {
		get_trans_pts_to_pts_3(
			pos, pos + dir, pos + up, pos + right,
			vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, -1.0), vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0),
			res
		);
	}
	inline void get_trans_camview_3(const camera_spec &cam, mat4 &res) {
		get_trans_camview_3(cam.pos, cam.forward, cam.up, cam.right_cache, res);
	}
	inline void get_trans_frustrum_3(rtt2_float hori_fov, rtt2_float aspect_ratio, rtt2_float znear, rtt2_float zfar, mat4 &res) {
		rtt2_float nfdist = zfar - znear, rdist = nfdist / std::tan(0.5 * hori_fov);
		res[0][0] = rdist;
		res[1][1] = rdist / aspect_ratio;
		res[2][3] = -nfdist;
		res[2][2] = zfar;
		res[3][2] = znear * zfar;
		res[0][1] = res[0][2] = res[0][3] =
			res[1][0] = res[1][2] = res[1][3] =
			res[2][0] = res[2][1] =
			res[3][0] = res[3][1] = res[3][3] = 0.0;
	}
	inline void get_trans_frustrum_3(const camera_spec &cam, mat4 &res) {
		get_trans_frustrum_3(cam.hori_fov, cam.aspect_ratio, cam.znear, cam.zfar, res);
	}
}
