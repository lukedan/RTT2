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
	struct texture {
	public:
		texture() : _w(0), _h(0), _arr(nullptr), _carr_cache(nullptr), _own(true) {
		}
		texture(size_t w, size_t h) : _w(w), _h(h), _arr(new device_color[_w * _h]), _carr_cache(nullptr), _own(true) {
		}
		texture(const texture&) = delete;
		texture &operator =(const texture&) = delete;
		~texture() {
			free_mem();
		}

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
		void clamp_coords(int &v, size_t max, uv_clamp_mode mode) const {
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
		void sample(const vec2 &uv, color_vec &res, uv_clamp_mode clampm, sample_mode sampm) const {
			switch (sampm) {
				case sample_mode::nearest:
				{
					int x = static_cast<int>(uv.x * _w), y = static_cast<int>(uv.y * _h);
					clamp_coords(x, _w, clampm);
					clamp_coords(y, _h, clampm);
					fetch(static_cast<size_t>(x), static_cast<size_t>(y), res);
					break;
				}
				case sample_mode::bilinear:
				{
					rtt2_float xf = uv.x * _w - 0.5, yf = uv.y * _h - 0.5;
					int x = static_cast<int>(xf), y = static_cast<int>(yf), x1 = x + 1, y1 = y + 1;
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
			free_mem();
			_w = w;
			_h = h;
			_arr = new device_color[_w * _h];
			_carr_cache = nullptr;
		}
		void reset() {
			free_mem();
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
		int ppm_get_next_digit(file_access &acc) {
			int c = EOF;
			while (acc) {
				c = acc.get_char();
				if (c == '#') {
					while (acc) {
						if (acc.get_char() == '\n') {
							break;
						}
					}
				} else if (c >= '0' && c <= '9') {
					return c;
				}
			}
			return c;
		}
		template <typename I> bool ppm_get_next_num(file_access &acc, I &res) {
			int v = ppm_get_next_digit(acc);
			if (v < '0' || v > '9') {
				return false;
			}
			res = v - '0';
			while (acc) {
				int c = acc.get_char();
				if (c >= '0' && c <= '9') {
					res = res * 10 + (c - '0');
				} else {
					acc.unget_char(c);
					break;
				}
			}
			return true;
		}
	public:
		void load_ppm(file_access &fin) {
			while (fin && fin.get_char() != 'P') {
			}
			size_t w, h, maxv;
			switch (fin.get_char()) {
				case '3':
					ppm_get_next_num(fin, w);
					ppm_get_next_num(fin, h);
					reset(w, h);
					ppm_get_next_num(fin, maxv);
					for (size_t y = 0; y < _h; ++y) {
						device_color *cur = get_at(0, _h - y - 1);
						for (size_t x = 0; x < _w; ++x, ++cur) {
							size_t r, g, b;
							ppm_get_next_num(fin, r);
							ppm_get_next_num(fin, g);
							ppm_get_next_num(fin, b);
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
					ppm_get_next_num(fin, w);
					ppm_get_next_num(fin, h);
					reset(w, h);
					ppm_get_next_num(fin, maxv);
					fin.get_char(); // get rid of the \n
					for (size_t y = 0; y < _h; ++y) {
						device_color *cur = get_at(0, _h - y - 1);
						for (size_t x = 0; x < _w; ++x, ++cur) {
							char
								r = static_cast<char>(fin.get_char()),
								g = static_cast<char>(fin.get_char()),
								b = static_cast<char>(fin.get_char());
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
		void save_ppm(file_access &out) const {
			out.write("P3\n%u %u\n255\n", _w, _h);
			for (size_t y = _h; y > 0; ) {
				device_color *cur = _arr + _w * --y;
				for (size_t x = 0; x < _w; ++x, ++cur) {
					out.write("%hhu %hhu %hhu \t", cur->get_r(), cur->get_g(), cur->get_b());
				}
				out.write("\n");
			}
		}

		void from_buffer(const buffer_set &buf) {
			reset();
			from_buffer_nocheck(buf);
		}
		void from_buffer_nocheck(const buffer_set &buf) {
			_w = buf.w;
			_h = buf.h;
			_arr = buf.color_arr;
			_own = false;
		}
	protected:
		size_t _w, _h;
		device_color *_arr;
		color_vec *_carr_cache;
		bool _own;

		void free_mem() {
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