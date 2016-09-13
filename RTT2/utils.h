#pragma once

#include <cstdio>
#include <deque>
#include <algorithm>
#include <atomic>

#include <Windows.h>

#define RTT2_EXPAND(X) X
#define RTT2_CONCAT(A, B) A##B

#define RTT2_PI 3.1415926535897
#define RTT2_SQRT2 1.4142135623731
#define RTT2_SQRT3 1.7320508075689
#define RTT2_SQRT5 2.2360679774998

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

	template <typename T> inline const T &clamp(const T &v, const T &min, const T &max) {
		return (v > min ? (v < max ? v : max) : min);
	}
	void clamp_vec(vec2 &v, rtt2_float min, rtt2_float max) {
		v.x = clamp(v.x, min, max);
		v.y = clamp(v.y, min, max);
	}
	void clamp_vec(vec2 &v, const vec2 &min, const vec2 &max) {
		v.x = clamp(v.x, min.x, max.x);
		v.y = clamp(v.y, min.y, max.y);
	}
	void clamp_vec(vec3 &v, rtt2_float min, rtt2_float max) {
		v.x = clamp(v.x, min, max);
		v.y = clamp(v.y, min, max);
		v.z = clamp(v.z, min, max);
	}
	void clamp_vec(vec3 &v, const vec3 &min, const vec3 &max) {
		v.x = clamp(v.x, min.x, max.x);
		v.y = clamp(v.y, min.y, max.y);
		v.z = clamp(v.z, min.z, max.z);
	}
	void clamp_vec(vec4 &v, rtt2_float min, rtt2_float max) {
		v.x = clamp(v.x, min, max);
		v.y = clamp(v.y, min, max);
		v.z = clamp(v.z, min, max);
		v.w = clamp(v.w, min, max);
	}
	void clamp_vec(vec4 &v, const vec4 &min, const vec4 &max) {
		v.x = clamp(v.x, min.x, max.x);
		v.y = clamp(v.y, min.y, max.y);
		v.z = clamp(v.z, min.z, max.z);
		v.w = clamp(v.w, min.w, max.w);
	}

	void max_vec(vec2 &v, const vec2 &maxv) {
		v.x = std::max(v.x, maxv.x);
		v.y = std::max(v.y, maxv.y);
	}
	void max_vec(vec2 &v, rtt2_float maxv) {
		v.x = std::max(v.x, maxv);
		v.y = std::max(v.y, maxv);
	}
	void max_vec(vec3 &v, const vec3 &maxv) {
		v.x = std::max(v.x, maxv.x);
		v.y = std::max(v.y, maxv.y);
		v.z = std::max(v.z, maxv.z);
	}
	void max_vec(vec3 &v, rtt2_float maxv) {
		v.x = std::max(v.x, maxv);
		v.y = std::max(v.y, maxv);
		v.z = std::max(v.z, maxv);
	}
	void max_vec(vec4 &v, const vec4 &maxv) {
		v.x = std::max(v.x, maxv.x);
		v.y = std::max(v.y, maxv.y);
		v.z = std::max(v.z, maxv.z);
		v.w = std::max(v.w, maxv.w);
	}
	void max_vec(vec4 &v, rtt2_float maxv) {
		v.x = std::max(v.x, maxv);
		v.y = std::max(v.y, maxv);
		v.z = std::max(v.z, maxv);
		v.w = std::max(v.w, maxv);
	}

	void min_vec(vec2 &v, const vec2 &min) {
		v.x = std::min(v.x, min.x);
		v.y = std::min(v.y, min.y);
	}
	void min_vec(vec2 &v, rtt2_float min) {
		v.x = std::min(v.x, min);
		v.y = std::min(v.y, min);
	}
	void min_vec(vec3 &v, const vec3 &min) {
		v.x = std::min(v.x, min.x);
		v.y = std::min(v.y, min.y);
		v.z = std::min(v.z, min.z);
	}
	void min_vec(vec3 &v, rtt2_float min) {
		v.x = std::min(v.x, min);
		v.y = std::min(v.y, min);
		v.z = std::min(v.z, min);
	}
	void min_vec(vec4 &v, const vec4 &min) {
		v.x = std::min(v.x, min.x);
		v.y = std::min(v.y, min.y);
		v.z = std::min(v.z, min.z);
		v.w = std::min(v.w, min.w);
	}
	void min_vec(vec4 &v, rtt2_float min) {
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

#define RTT2_SORT3(ARR, MEM, CMP)         \
	{                                     \
		if (ARR[0]MEM CMP ARR[1]MEM) {    \
			std::swap(ARR[0], ARR[1]);    \
		}                                 \
		if (ARR[1]MEM CMP ARR[2]MEM) {    \
			std::swap(ARR[1], ARR[2]);    \
		}                                 \
		if (ARR[0]MEM CMP ARR[1]MEM) {    \
			std::swap(ARR[0], ARR[1]);    \
		}                                 \
	}                                     \

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

	struct file_access {
	public:
		file_access() : _f(nullptr) {
		}
		file_access(const std::string &file, const std::string &mode) {
#ifdef _MSC_VER
			fopen_s(&_f, file.c_str(), mode.c_str());
#else
			_f = std::fopen(file.c_str(), mode.c_str());
#endif
		}
		file_access(const file_access&) = delete;
		file_access &operator =(const file_access&) = delete;
		~file_access() {
			close();
		}

		void close() {
			if (_f) {
				std::fclose(_f);
				_f = nullptr;
			}
		}

		void write(const char *str, ...) {
			va_list args;
			va_start(args, str);
#ifdef _MSC_VER
			vfprintf_s(_f, str, args);
#else
			std::vfprintf(_f, str, args);
#endif
			va_end(args);
		}
		void read(const char *str, ...) {
			va_list args;
			va_start(args, str);
#ifdef _MSC_VER
			vfscanf_s(_f, str, args);
#else
			std::vfscanf(_f, str, args);
#endif
			va_end(args);
		}
		int get_char() {
			return std::fgetc(_f);
		}
		void unget_char(int c) {
			std::ungetc(c, _f);
		}
		void put_char(int c) {
			std::fputc(c, _f);
		}

		void get_line(std::string &str, char delim = '\n') {
			str = "";
			for (int x = get_char(); good() && x != delim; str += static_cast<char>(x), x = get_char()) {
			}
		}

		void write_bin(const void *obj, size_t sz) {
			std::fwrite(obj, sz, 1, _f);
		}
		void read_bin(void *buf, size_t sz) {
#ifdef _MSC_VER
			fread_s(buf, sz, sz, 1, _f);
#else
			fread(buf, sz, 1, _f);
#endif
		}
		template <typename T> void write_object_bin(const T &obj) {
			write_bin(&obj, sizeof(obj));
		}
		template <typename T> void read_object_bin(T &obj) {
			read_bin(&obj, sizeof(obj));
		}

		bool good() const {
			return _f && std::ferror(_f) == 0 && std::feof(_f) == 0;
		}
		bool eof() const {
			return _f == nullptr || std::feof(_f) != 0;
		}
		operator bool() const {
			return good();
		}

		FILE *handle() const {
			return _f;
		}
	protected:
		FILE *_f;
	};

	enum class task_status : unsigned char {
		stopped,
		running,
		cancelled
	};
	struct task_lock {
	public:
		bool try_start() {
			task_status exp(task_status::stopped);
			return _lock.compare_exchange_strong(exp, task_status::running);
		}
		bool check_cancellation_and_stop() {
			task_status exp(task_status::cancelled);
			return _lock.compare_exchange_strong(exp, task_status::stopped);
		}

		void set_status(task_status ts) {
			_lock.store(ts);
		}
		task_status get_status() const {
			return _lock.load();
		}
		void on_stopped() {
			set_status(task_status::stopped);
		}
		bool cancel() {
			task_status exp(task_status::running);
			return _lock.compare_exchange_strong(exp, task_status::cancelled);
		}
		void wait() {
			while (_lock.load() == task_status::cancelled) {
			}
		}
		bool cancel_and_wait() {
			if (cancel()) {
				wait();
				return true;
			}
			return false;
		}
	protected:
		std::atomic<task_status> _lock{ task_status::stopped };
	};
	struct task_toggler {
	public:
		typedef void (*callback)();

		task_toggler(callback c, callback b) : on_clear(c), on_blocked(b) {
		}

		callback on_clear, on_blocked;

		task_toggler *affected = nullptr;

		void add_barrier() {
			if ((++_dep) == 1) {
				if (affected) {
					affected->add_barrier();
				}
				on_blocked();
			}
		}
		void remove_barrier() {
			if ((--_dep) == 0) {
				on_clear();
				if (affected) {
					affected->remove_barrier();
				}
			}
		}
	protected:
		size_t _dep = 0;
	};

	// functions for debugging
	template <size_t Dim> struct sqrmat;
	template <size_t Dim> inline void print_mat(const sqrmat<Dim> &mat, size_t fw = 10, char pre = '[', char end = ']') {
		static char buffer[10];

		std::snprintf(buffer, 10, "%%%uf", fw);
		for (size_t y = 0; y < Dim; ++y) {
			std::printf("%c", pre);
			for (size_t x = 0; x < Dim; ++x) {
				std::printf(buffer, mat[x][y]);
			}
			std::printf("%c\n", end);
		}
	}
}