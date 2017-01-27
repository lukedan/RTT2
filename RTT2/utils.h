#pragma once

#include <deque>
#include <algorithm>
#include <atomic>
#include <iomanip>
#include <Windows.h>

#include "vec.h"

#define RTT2_EXPAND(X) X
#define RTT2_CONCAT(A, B) A##B

// FUCK WINDOWS
#undef min
#undef max

namespace rtt2 {
	inline long long get_time() {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return li.QuadPart;
	}
	inline long long get_timer_freq() {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		return li.QuadPart;
	}

	inline bool is_key_down(int vk) {
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}
	inline void get_mouse_global_pos(int &x, int &y) {
		POINT p;
		GetCursorPos(&p);
		x = p.x;
		y = p.y;
	}
	inline void set_mouse_global_pos(int x, int y) {
		SetCursorPos(x, y);
	}

	typedef rtt2_float(*randomizer)();
	inline vec3 get_random_direction_on_sphere(randomizer rand) {
		rtt2_float a1 = rand() * RTT2_PI, a2 = rand() * 2.0 * RTT2_PI;
		rtt2_float cv = std::sin(a1);
		return vec3(std::cos(a1), cv * std::sin(a2), cv * std::cos(a2));
	}
	inline vec3 get_random_direction_on_hemisphere(const vec3 &normal, randomizer rand) {
		vec3 vp1, vp2;
		normal.get_max_prp(vp1);
		vp1.set_length(1.0);
		vec3::cross_ref(normal, vp1, vp2);
		rtt2_float hv = rand() * RTT2_PI, vv = rand() * 2.0 * RTT2_PI;
		return normal * std::sin(hv) + std::cos(hv) * (std::sin(vv) * vp1 + std::cos(vv) * vp2);
	}

	template <typename T> inline const T &clamp(const T &v, const T &min, const T &max) {
		return (v > min ? (v < max ? v : max) : min);
	}
	inline void clamp_vec(vec2 &v, rtt2_float min, rtt2_float max) {
		v.x = clamp(v.x, min, max);
		v.y = clamp(v.y, min, max);
	}
	inline void clamp_vec(vec2 &v, const vec2 &min, const vec2 &max) {
		v.x = clamp(v.x, min.x, max.x);
		v.y = clamp(v.y, min.y, max.y);
	}
	inline void clamp_vec(vec3 &v, rtt2_float min, rtt2_float max) {
		v.x = clamp(v.x, min, max);
		v.y = clamp(v.y, min, max);
		v.z = clamp(v.z, min, max);
	}
	inline void clamp_vec(vec3 &v, const vec3 &min, const vec3 &max) {
		v.x = clamp(v.x, min.x, max.x);
		v.y = clamp(v.y, min.y, max.y);
		v.z = clamp(v.z, min.z, max.z);
	}
	inline void clamp_vec(vec4 &v, rtt2_float min, rtt2_float max) {
		v.x = clamp(v.x, min, max);
		v.y = clamp(v.y, min, max);
		v.z = clamp(v.z, min, max);
		v.w = clamp(v.w, min, max);
	}
	inline void clamp_vec(vec4 &v, const vec4 &min, const vec4 &max) {
		v.x = clamp(v.x, min.x, max.x);
		v.y = clamp(v.y, min.y, max.y);
		v.z = clamp(v.z, min.z, max.z);
		v.w = clamp(v.w, min.w, max.w);
	}

	inline void max_vec(vec2 &v, const vec2 &maxv) {
		v.x = std::max(v.x, maxv.x);
		v.y = std::max(v.y, maxv.y);
	}
	inline void max_vec(vec2 &v, rtt2_float maxv) {
		v.x = std::max(v.x, maxv);
		v.y = std::max(v.y, maxv);
	}
	inline void max_vec(vec3 &v, const vec3 &maxv) {
		v.x = std::max(v.x, maxv.x);
		v.y = std::max(v.y, maxv.y);
		v.z = std::max(v.z, maxv.z);
	}
	inline void max_vec(vec3 &v, rtt2_float maxv) {
		v.x = std::max(v.x, maxv);
		v.y = std::max(v.y, maxv);
		v.z = std::max(v.z, maxv);
	}
	inline void max_vec(vec4 &v, const vec4 &maxv) {
		v.x = std::max(v.x, maxv.x);
		v.y = std::max(v.y, maxv.y);
		v.z = std::max(v.z, maxv.z);
		v.w = std::max(v.w, maxv.w);
	}
	inline void max_vec(vec4 &v, rtt2_float maxv) {
		v.x = std::max(v.x, maxv);
		v.y = std::max(v.y, maxv);
		v.z = std::max(v.z, maxv);
		v.w = std::max(v.w, maxv);
	}

	inline void min_vec(vec2 &v, const vec2 &min) {
		v.x = std::min(v.x, min.x);
		v.y = std::min(v.y, min.y);
	}
	inline void min_vec(vec2 &v, rtt2_float min) {
		v.x = std::min(v.x, min);
		v.y = std::min(v.y, min);
	}
	inline void min_vec(vec3 &v, const vec3 &min) {
		v.x = std::min(v.x, min.x);
		v.y = std::min(v.y, min.y);
		v.z = std::min(v.z, min.z);
	}
	inline void min_vec(vec3 &v, rtt2_float min) {
		v.x = std::min(v.x, min);
		v.y = std::min(v.y, min);
		v.z = std::min(v.z, min);
	}
	inline void min_vec(vec4 &v, const vec4 &min) {
		v.x = std::min(v.x, min.x);
		v.y = std::min(v.y, min.y);
		v.z = std::min(v.z, min.z);
		v.w = std::min(v.w, min.w);
	}
	inline void min_vec(vec4 &v, rtt2_float min) {
		v.x = std::min(v.x, min);
		v.y = std::min(v.y, min);
		v.z = std::min(v.z, min);
		v.w = std::min(v.w, min);
	}

	template <typename T> inline bool clip_line_onedir(T &fx, T &fy, T &tx, T &ty, const T &xmin, const T &xmax) {
#define RTT2_CLIP_LINE_ONEDIR_FIXUP(X, Y, V, K)    \
	(Y) += (K) * ((V) - (X));				       \
	(X) = (V)								       \

		if (fx < tx) {
			if (tx < xmin || fx > xmax) {
				return false;
			}
			T k = (ty - fy) / (tx - fx);
			if (fx < xmin) {
				RTT2_CLIP_LINE_ONEDIR_FIXUP(fx, fy, xmin, k);
			}
			if (tx > xmax) {
				RTT2_CLIP_LINE_ONEDIR_FIXUP(tx, ty, xmax, k);
			}
		} else {
			if (fx < xmin || tx > xmax) {
				return false;
			}
			T k = (ty - fy) / (tx - fx);
			if (fx > xmax) {
				RTT2_CLIP_LINE_ONEDIR_FIXUP(fx, fy, xmax, k);
			}
			if (tx < xmin) {
				RTT2_CLIP_LINE_ONEDIR_FIXUP(tx, ty, xmin, k);
			}
		}
		return true;
	}

	struct hit_test_ray_triangle_results {
		rtt2_float t, u, v;
	};
	inline bool hit_test_ray_triangle(
		const vec3 &rs, const vec3 &rd, const vec3 &p1, const vec3 &p2, const vec3 &p3,
		hit_test_ray_triangle_results &result
	) {
		vec3 e1 = p2 - p1, e2 = p3 - p1, p = vec3::cross(rd, e2);
		rtt2_float
			det = vec3::dot(p, e1);
		vec3 t = rs - p1;
		if (det < 0.0) {
			det = -det;
			t = -t;
		}
		result.u = vec3::dot(p, t);
		if (result.u < 0.0 || result.u > det) {
			return false;
		}
		vec3 q = vec3::cross(t, e1);
		result.v = vec3::dot(q, rd);
		if (result.v < 0.0 || result.u + result.v > det) {
			return false;
		}
		result.t = vec3::dot(q, e2);
		if (result.t < 0.0) {
			return false;
		}
		det = 1.0 / det;
		result.t *= det;
		result.u *= det;
		result.v *= det;
		return true;
	}

	template <typename T, typename Cmp> inline void sort3(T *arr, Cmp cmp = Cmp()) {
		if (cmp(arr[0], arr[1])) {
			std::swap(arr[0], arr[1]);
		}
		if (cmp(arr[1], arr[2])) {
			std::swap(arr[1], arr[2]);
		}
		if (cmp(arr[0], arr[1])) {
			std::swap(arr[0], arr[1]);
		}
	}

	struct fps_counter {
	public:
		fps_counter() : _freq(get_timer_freq()), _wnd(_freq) {
		}

		void update() {
			long long cur = get_time(), pvt = cur - _wnd;
			if (_rec.size() > 0) {
				_sf = _freq / static_cast<rtt2_float>(cur - _rec.back());
			}
			_rec.push_back(cur);
			while (_rec.front() < pvt) {
				_lastpop = _rec.front();
				_rec.pop_front();
			}
		}

		void set_window(rtt2_float secs) {
			_wnd = static_cast<long long>(_freq * secs);
		}

		rtt2_float get_fps() const {
			if (_rec.size() == 0) {
				return 0.0;
			}
			return (_rec.size() - (_rec.back() - _wnd - _lastpop) / static_cast<rtt2_float>(_rec.front() - _lastpop)) * (_freq / static_cast<rtt2_float>(_wnd));
		}
		rtt2_float get_singleframe_fps() const {
			return _sf;
		}
		rtt2_float get_debug_fps() const {
			return _rec.size() * (_freq / static_cast<rtt2_float>(_wnd));
		}
	protected:
		std::deque<long long> _rec;
		long long _freq, _wnd, _lastpop;
		rtt2_float _sf;
	};

	struct stopwatch {
	public:
		stopwatch() : _freq(get_timer_freq()), _last(get_time()) {
		}

		long long tick() {
			long long cur = get_time(), res = cur - _last;
			_last = cur;
			return res;
		}
		rtt2_float tick_in_seconds() {
			return tick() / static_cast<rtt2_float>(_freq);
		}
	protected:
		long long _freq, _last;
	};

	struct key_monitor {
	public:
		typedef void(*handle)();

		void update() {
			if (is_key_down(key)) {
				if (!_ld) {
					_ld = true;
					if (on_down) {
						on_down();
					}
				}
			} else if (_ld) {
				_ld = false;
				if (on_up) {
					on_up();
				}
			}
		}

		void set(int k, handle d, handle u) {
			key = k;
			on_down = d;
			on_up = u;
		}

		bool down() const {
			return _ld;
		}

		int key;
		handle on_down, on_up;
	protected:
		bool _ld = false;
	};

	// functions for debugging
	template <size_t Dim> struct sqrmat;
	template <size_t Dim> inline void print_mat(const sqrmat<Dim> &mat, size_t fw = 10, char pre = '[', char end = ']') {
		for (size_t y = 0; y < Dim; ++y) {
			std::cout << pre;
			for (size_t x = 0; x < Dim; ++x) {
				std::cout << std::setw(fw) << mat[x][y];
			}
			std::cout << end;
		}
	}
}