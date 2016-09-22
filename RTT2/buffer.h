#pragma once

#include <cstring>

#include "color.h"

namespace rtt2 {
	class sys_color_buffer {
	public:
		typedef device_color element_type;

		sys_color_buffer(HDC hdc, size_t w, size_t h) : _w(w), _h(h), _dc(CreateCompatibleDC(hdc)) {
			BITMAPINFO info;
			std::memset(&info, 0, sizeof(BITMAPINFO));
			info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info.bmiHeader.biWidth = _w;
			info.bmiHeader.biHeight = _h;
			info.bmiHeader.biPlanes = 1;
			info.bmiHeader.biBitCount = 32;
			info.bmiHeader.biCompression = BI_RGB;
			_bmp = CreateDIBSection(_dc, &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&_arr), nullptr, 0);
			_old = static_cast<HBITMAP>(SelectObject(_dc, _bmp));
		}
		sys_color_buffer(const sys_color_buffer&) = delete;
		sys_color_buffer &operator =(const sys_color_buffer&) = delete;
		~sys_color_buffer() {
			SelectObject(_dc, _old);
			DeleteObject(_bmp);
			DeleteDC(_dc);
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
		device_color *get_at(size_t x, size_t y) const {
#ifdef DEBUG
			if (x >= _w || y >= _h) {
				throw std::range_error("invalid coords");
			}
#endif
			return _arr + (_w * y + x);
		}

		void display(HDC hdc) const {
			BitBlt(hdc, 0, 0, _w, _h, _dc, 0, 0, SRCCOPY);
		}

		void fetch(size_t x, size_t y, device_color &dc) const {
			dc = *get_at(x, y);
		}
	protected:
		size_t _w, _h;
		HBITMAP _bmp, _old;
		HDC _dc;
		device_color *_arr;
	};

	template <size_t sz> struct sized_object {
	private:
		std::uint8_t _placeholder[sz];
	};
	struct dyn_sized_object {
	public:
		dyn_sized_object() : _placeholder(nullptr) {
		}
		dyn_sized_object(size_t sz) : _placeholder(std::malloc(sz)) {
		}
		dyn_sized_object(const dyn_sized_object&) = delete;
		dyn_sized_object &operator =(const dyn_sized_object&) = delete;
		~dyn_sized_object() {
			clear();
		}

		void *get() const {
			return _placeholder;
		}

		void reset(size_t sz) {
			clear();
			_placeholder = std::malloc(sz);
		}

		void clear() {
			if (_placeholder) {
				std::free(_placeholder);
				_placeholder = nullptr;
			}
		}
	private:
		void *_placeholder;
	};
	struct dyn_mem_arr {
	public:
		dyn_mem_arr() : _arr(nullptr) {
		}
		dyn_mem_arr(size_t sz, size_t count) : _block(sz), _arr(std::malloc(_block * count)) {
		}
		dyn_mem_arr(const dyn_mem_arr&) = delete;
		dyn_mem_arr &operator =(const dyn_mem_arr&) = delete;
		~dyn_mem_arr() {
			clear();
		}

		void *get_arr() const {
			return _arr;
		}
		void *get_at(size_t id) const {
			return reinterpret_cast<void*>(reinterpret_cast<size_t>(_arr) + id * _block);
		}

		void reset(size_t block, size_t count) {
			clear();
			_block = block;
			_arr = std::malloc(_block * count);
		}

		void clear() {
			if (_arr) {
				std::free(_arr);
				_arr = nullptr;
			}
		}
	protected:
		size_t _block;
		void *_arr;
	};
	template <typename T> class mem_buffer {
	public:
		typedef T element_type;

		mem_buffer(size_t w, size_t h) : _w(w), _h(h), _arr(new T[_w * _h]) {
		}
		mem_buffer(const mem_buffer&) = delete;
		mem_buffer &operator =(const mem_buffer&) = delete;
		~mem_buffer() {
			delete[] _arr;
		}

		T *get_arr() const {
			return _arr;
		}
		size_t get_w() const {
			return _w;
		}
		size_t get_h() const {
			return _h;
		}

		void clear(const T &obj) {
			T *cur = _arr;
			for (size_t i = _w * _h; i > 0; --i, ++cur) {
				*cur = obj;
			}
		}

		void fetch(size_t x, size_t y, T &res) const {
			res = *get_at(x, y);
		}

		T *get_at(size_t x, size_t y) const {
#ifdef DEBUG
			if (x >= _w || y >= _h) {
				throw std::range_error("invalid coords");
			}
#endif
			return _arr + (_w * y + x);
		}
	protected:
		size_t _w, _h;
		T *_arr;
	};
	typedef mem_buffer<device_color> mem_color_buffer;
	typedef mem_buffer<rtt2_float> mem_depth_buffer;
	typedef mem_buffer<unsigned char> mem_stencil_buffer;

	class buffer_set {
	public:
		buffer_set() = default;
		buffer_set(size_t ww, size_t hh, device_color *c, rtt2_float *d, unsigned char *s) : w(ww), h(hh), color_arr(c), depth_arr(d), stencil_arr(s) {
		}

		size_t w, h;
		device_color *color_arr;
		rtt2_float *depth_arr;
		unsigned char *stencil_arr;

		void set(size_t ww, size_t hh, device_color *c, rtt2_float *d, unsigned char *s) {
			w = ww;
			h = hh;
			color_arr = c;
			depth_arr = d;
			stencil_arr = s;
		}

		template <typename T> T *get_at(size_t x, size_t y, T *arr) const {
#ifdef DEBUG
			if (x >= w || y >= h) {
				throw std::range_error("invalid coords");
			}
#endif
			return arr + (w * y + x);
		}

		void denormalize_scr_coord(rtt2_float &x, rtt2_float &y) const {
			x = (x + 1) * 0.5 * w;
			y = (y + 1) * 0.5 * h;
		}
		void denormalize_scr_coord(vec2 &r) const {
			denormalize_scr_coord(r.x, r.y);
		}
		void normalize_scr_coord(size_t x, size_t y, rtt2_float &rx, rtt2_float &ry) const {
			rx = (2 * x + 1) / static_cast<rtt2_float>(w) - 1.0;
			ry = (2 * y + 1) / static_cast<rtt2_float>(h) - 1.0;
		}
		void normalize_scr_coord(size_t x, size_t y, vec2 &r) const {
			normalize_scr_coord(x, y, r.x, r.y);
		}
	};

	template <typename U, typename V> void enlarged_copy(const U &src, V &dst) {
		rtt2_float
			xenl = src.get_w() / static_cast<rtt2_float>(dst.get_w()),
			yenl = src.get_h() / static_cast<rtt2_float>(dst.get_h());
		for (size_t y = 0; y < dst.get_h(); ++y) {
			for (size_t x = 0; x < dst.get_w(); ++x) {
				*dst.get_at(x, y) = *src.get_at(static_cast<size_t>((x + 0.5) * xenl), static_cast<size_t>((y + 0.5) * yenl));
			}
		}
	}
}
