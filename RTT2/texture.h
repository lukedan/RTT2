#pragma once

#include <string>
#include <fstream>

#include "color.h"
#include "buffer.h"

namespace rtt2 {
	enum class uv_clamp_mode {
		repeat,
		rev_repeat,
		repeat_border
	};
	enum class sample_mode {
		nearest,
		bilinear
	};

	inline void clamp_coords(int &v, size_t max, uv_clamp_mode mode) {
		switch (mode) {
			case uv_clamp_mode::repeat:
				v %= static_cast<int>(max);
				if (v < 0) {
					v += static_cast<int>(max);
				}
				break;
			case uv_clamp_mode::repeat_border:
				if (v < 0) {
					v = 0;
				} else if (static_cast<size_t>(v) >= max) {
					v = max - 1;
				}
				break;
			case uv_clamp_mode::rev_repeat:
				int dm = max * 2;
				v %= dm;
				if (v < 0) {
					v += dm;
				}
				if (static_cast<size_t>(v) >= max) {
					v = dm - v - 1;
				}
				break;
		}
	}
	template <typename U, typename V> inline void sample(const U &src, const vec2 &uv, V &res, uv_clamp_mode clampm, sample_mode sampm) {
		switch (sampm) {
			case sample_mode::nearest:
			{
				int x = static_cast<int>(std::floor(uv.x * src.get_w())), y = static_cast<int>(std::floor(uv.y * src.get_h()));
				clamp_coords(x, src.get_w(), clampm);
				clamp_coords(y, src.get_h(), clampm);
				src.fetch(static_cast<size_t>(x), static_cast<size_t>(y), res);
				break;
			}
			case sample_mode::bilinear:
			{
				rtt2_float xf = uv.x * src.get_w() - 0.5, yf = uv.y * src.get_h() - 0.5;
				int x = static_cast<int>(std::floor(xf)), y = static_cast<int>(std::floor(yf)), x1 = x + 1, y1 = y + 1;
				xf -= x;
				yf -= y;
				clamp_coords(x, src.get_w(), clampm);
				clamp_coords(x1, src.get_w(), clampm);
				clamp_coords(y, src.get_h(), clampm);
				clamp_coords(y1, src.get_h(), clampm);
				V v[4];
				src.fetch(x, y, v[0]);
				src.fetch(x1, y, v[1]);
				src.fetch(x, y1, v[2]);
				src.fetch(x1, y1, v[3]);
				res = v[0] + (v[1] - v[0]) * (1.0 - yf) * xf + (v[2] - v[0] + (v[3] - v[2]) * xf) * yf;
				break;
			}
		}
	}
	template <typename U, typename V> inline void sample(const U &src, const vec2 &uv, V &res) {
		sample<U, V>(src, uv, res, src.mode_uv, src.mode_sample);
	}

	struct texture {
	public:
		texture() : _w(0), _h(0), _arr(nullptr), _carr_cache(nullptr), _own(true) {
		}
		texture(size_t w, size_t h) : _w(w), _h(h), _arr(new device_color[_w * _h]), _carr_cache(nullptr), _own(true) {
		}
		texture(const texture&) = delete;
		texture &operator =(const texture&) = delete;
		~texture() {
			_free_mem();
		}

		uv_clamp_mode mode_uv = uv_clamp_mode::repeat;
		sample_mode mode_sample = sample_mode::bilinear;

		size_t get_w() const {
			return _w;
		}
		size_t get_h() const {
			return _h;
		}
		device_color *get_arr() const {
			return _arr;
		}
		color_vec *get_arr_vec() const {
			return _carr_cache;
		}
		device_color *get_at(size_t x, size_t y) const {
#ifdef DEBUG
			if (x >= _w || y >= _h) {
				throw std::range_error("texel fetch coord out of bounds");
			}
#endif
			return _arr + (_w * y + x);
		}
		color_vec *get_vec_at(size_t x, size_t y) const {
#ifdef DEBUG
			if (x >= _w || y >= _h) {
				throw std::range_error("texel fetch coord out of bounds");
			}
#endif
			return _carr_cache + (_w * y + x);
		}
		void fetch(size_t x, size_t y, color_vec &res) const {
#ifdef DEBUG
			if (x >= _w || y >= _h) {
				throw std::range_error("texel fetch coord out of bounds");
			}
#endif
			res = *get_vec_at(x, y);
		}
		void grad_x(const vec2 &uv, color_vec &res, uv_clamp_mode clampm) const {
			int x = static_cast<int>(std::floor(uv.x * _w)), y = static_cast<int>(std::floor(uv.y * _h)), x1 = x + 1;
			--x;
			clamp_coords(x, _w, clampm);
			clamp_coords(x1, _w, clampm);
			clamp_coords(y, _h, clampm);
			color_vec t;
			fetch(static_cast<size_t>(x), static_cast<size_t>(y), t);
			fetch(static_cast<size_t>(x1), static_cast<size_t>(y), res);
			res -= t;
			res *= 0.5;
		}
		void grad_x(const vec2 &uv, color_vec &res) const {
			grad_x(uv, res, mode_uv);
		}
		void grad_y(const vec2 &uv, color_vec &res, uv_clamp_mode clampm) const {
			int x = static_cast<int>(std::floor(uv.x * _w)), y = static_cast<int>(std::floor(uv.y * _h)), y1 = y + 1;
			--y;
			clamp_coords(x, _w, clampm);
			clamp_coords(y, _h, clampm);
			clamp_coords(y1, _h, clampm);
			color_vec t;
			fetch(static_cast<size_t>(x), static_cast<size_t>(y), t);
			fetch(static_cast<size_t>(x), static_cast<size_t>(y1), res);
			res -= t;
			res *= 0.5;
		}
		void grad_y(const vec2 &uv, color_vec &res) const {
			grad_y(uv, res, mode_uv);
		}
		void sample(const vec2 &uv, color_vec &res, uv_clamp_mode clampm, sample_mode sampm) const {
			switch (sampm) {
				case sample_mode::nearest:
				{
					int x = static_cast<int>(std::floor(uv.x * _w)), y = static_cast<int>(std::floor(uv.y * _h));
					clamp_coords(x, _w, clampm);
					clamp_coords(y, _h, clampm);
					fetch(static_cast<size_t>(x), static_cast<size_t>(y), res);
					break;
				}
				case sample_mode::bilinear:
				{
					rtt2_float xf = uv.x * _w - 0.5, yf = uv.y * _h - 0.5;
					int x = static_cast<int>(std::floor(xf)), y = static_cast<int>(std::floor(yf)), x1 = x + 1, y1 = y + 1;
					xf -= x;
					yf -= y;
					clamp_coords(x, _w, clampm);
					clamp_coords(x1, _w, clampm);
					clamp_coords(y, _h, clampm);
					clamp_coords(y1, _h, clampm);
					vec4 v[4];
					fetch(x, y, v[0]);
					fetch(x1, y, v[1]);
					fetch(x, y1, v[2]);
					fetch(x1, y1, v[3]);
					res = v[0] + (v[1] - v[0]) * (1.0 - yf) * xf + (v[2] - v[0] + (v[3] - v[2]) * xf) * yf;
					break;
				}
			}
		}

		void reset(size_t w, size_t h) {
			_free_mem();
			_w = w;
			_h = h;
			_arr = new device_color[_w * _h];
			_carr_cache = nullptr;
		}
		void reset() {
			_free_mem();
			_arr = nullptr;
			_carr_cache = nullptr;
		}

		void make_float_cache() {
			if (_carr_cache) {
				delete[] _carr_cache;
			}
			make_float_cache_nocheck();
		}
		void make_float_cache_nocheck() {
			_carr_cache = new color_vec[_w * _h];
			color_vec *c = _carr_cache;
			device_color *d = _arr;
			for (size_t i = _w * _h; i > 0; --i, ++c, ++d) {
				d->to_vec4(*c);
			}
		}
	protected:
		int _ppm_get_next_digit(std::istream &acc) {
			int c = std::ifstream::traits_type::eof();
			while (acc) {
				c = acc.get();
				if (c == '#') {
					while (acc) {
						if (acc.get() == '\n') {
							break;
						}
					}
				} else if (c >= '0' && c <= '9') {
					return c;
				}
			}
			return c;
		}
		template <typename I> bool _ppm_get_next_num(std::istream &acc, I &res) {
			int v = _ppm_get_next_digit(acc);
			if (v < '0' || v > '9') {
				return false;
			}
			res = v - '0';
			while (acc) {
				int c = acc.get();
				if (c >= '0' && c <= '9') {
					res = res * 10 + (c - '0');
				} else {
					acc.unget();
					break;
				}
			}
			return true;
		}
	public:
		void load_ppm(std::istream &fin) {
			while (fin && fin.get() != 'P') {
			}
			size_t w, h, maxv;
			switch (fin.get()) {
				case '3':
					_ppm_get_next_num(fin, w);
					_ppm_get_next_num(fin, h);
					reset(w, h);
					_ppm_get_next_num(fin, maxv);
					for (size_t y = 0; y < _h; ++y) {
						device_color *cur = get_at(0, _h - y - 1);
						for (size_t x = 0; x < _w; ++x, ++cur) {
							size_t r, g, b;
							_ppm_get_next_num(fin, r);
							_ppm_get_next_num(fin, g);
							_ppm_get_next_num(fin, b);
							cur->set(
								255,
								static_cast<unsigned char>(255 * (r / static_cast<rtt2_float>(maxv))),
								static_cast<unsigned char>(255 * (g / static_cast<rtt2_float>(maxv))),
								static_cast<unsigned char>(255 * (b / static_cast<rtt2_float>(maxv)))
							);
						}
					}
					break;
				case '6':
					_ppm_get_next_num(fin, w);
					_ppm_get_next_num(fin, h);
					reset(w, h);
					_ppm_get_next_num(fin, maxv);
					fin.get(); // get rid of the \n
					for (size_t y = 0; y < _h; ++y) {
						device_color *cur = get_at(0, _h - y - 1);
						for (size_t x = 0; x < _w; ++x, ++cur) {
							unsigned char
								r = static_cast<unsigned char>(fin.get()),
								g = static_cast<unsigned char>(fin.get()),
								b = static_cast<unsigned char>(fin.get());
							cur->set(
								255,
								static_cast<unsigned char>(255 * (r / static_cast<rtt2_float>(maxv))),
								static_cast<unsigned char>(255 * (g / static_cast<rtt2_float>(maxv))),
								static_cast<unsigned char>(255 * (b / static_cast<rtt2_float>(maxv)))
							);
						}
						//fin.get_char();
					}
					break;
#ifdef DEBUG
				default:
					throw std::invalid_argument("image mode not implemented");
#endif
			}
		}
		void save_ppm(std::ostream &out) const {
			out << "P3\n" << _w << " " << _h << "\n255\n";
			for (size_t y = _h; y > 0; ) {
				device_color *cur = _arr + _w * --y;
				for (size_t x = 0; x < _w; ++x, ++cur) {
					out << cur->get_r() << " " << cur->get_g() << " " << cur->get_b() << " \t";
				}
				out << "\n";
			}
		}

		template <typename T> void from_buffer(const T &buf) {
			reset();
			from_buffer_nocheck(buf);
		}
		template <typename T> void from_buffer_nocheck(const T &buf) {
			_w = buf.get_w();
			_h = buf.get_h();
			_arr = buf.get_arr();
			_own = false;
		}
	protected:
		size_t _w, _h;
		device_color *_arr = nullptr;
		color_vec *_carr_cache = nullptr;
		bool _own;

		void _free_mem() {
			if (_arr) {
				if (_own) {
					delete[] _arr;
				}
				if (_carr_cache) {
					delete[] _carr_cache;
				}
			}
		}
	};
}