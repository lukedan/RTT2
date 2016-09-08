#pragma once

#include <cstring>

#include "color.h"

namespace rtt2 {
	class sys_color_buffer {
	public:
		typedef device_color element_type;

		sys_color_buffer(HDC hdc, size_t w, size_t h) : _dc(CreateCompatibleDC(hdc)) {
			BITMAPINFO info;
			std::memset(&info, 0, sizeof(BITMAPINFO));
			info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info.bmiHeader.biWidth = w;
			info.bmiHeader.biHeight = h;
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

		device_color *get_arr() const {
			return _arr;
		}

		void display(HDC hdc, size_t w, size_t h) const {
			BitBlt(hdc, 0, 0, w, h, _dc, 0, 0, SRCCOPY);
		}
	protected:
		HBITMAP _bmp, _old;
		HDC _dc;
		device_color *_arr;
	};

	template <typename T> class mem_buffer {
	public:
		typedef T element_type;

		mem_buffer(size_t w, size_t h) : _arr(new T[w * h]) {
		}
		mem_buffer(const mem_buffer&) = delete;
		mem_buffer &operator =(const mem_buffer&) = delete;
		~mem_buffer() {
			delete[] _arr;
		}

		T *get_arr() const {
			return _arr;
		}
	protected:
		T *_arr;
	};
	typedef mem_buffer<device_color> mem_color_buffer;
	typedef mem_buffer<rtt2_float> mem_depth_buffer;
	typedef mem_buffer<unsigned char> mem_stencil_buffer;

	enum class buffer_set_type {
		nograph,
		sys,
		mem
	};
	class buffer_set {
	public:
		buffer_set(size_t w, size_t h) : _w(w), _h(h), _dbuf(_w, _h), _sbuf(_w, _h) {
		}
		~buffer_set() {
			switch (_type) {
				case buffer_set_type::sys:
					delete static_cast<sys_color_buffer*>(_cbuf);
					break;
				case buffer_set_type::mem:
					delete static_cast<mem_color_buffer*>(_cbuf);
					break;
			}
		}

		void make_sys_buffer_set(HDC dc) {
			sys_color_buffer *buf = new sys_color_buffer(dc, _w, _h);
			_cbuf = buf;
			_carr = buf->get_arr();
			_type = buffer_set_type::sys;
		}
		void make_mem_buffer_set() {
			mem_color_buffer *buf = new mem_color_buffer(_w, _h);
			_cbuf = buf;
			_carr = buf->get_arr();
			_type = buffer_set_type::mem;
		}

		void denormalize_scr_coord(rtt2_float &x, rtt2_float &y) const {
			x = (x + 1) * 0.5 * _w;
			y = (y + 1) * 0.5 * _h;
		}
		void denormalize_scr_coord(vec2 &r) const {
			denormalize_scr_coord(r.x, r.y);
		}
		void normalize_scr_coord(size_t x, size_t y, rtt2_float &rx, rtt2_float &ry) const {
			rx = (2 * x + 1) / static_cast<rtt2_float>(_w) - 1.0;
			ry = (2 * y + 1) / static_cast<rtt2_float>(_h) - 1.0;
		}
		void normalize_scr_coord(size_t x, size_t y, vec2 &r) const {
			normalize_scr_coord(x, y, r.x, r.y);
		}

		size_t get_w() const {
			return _w;
		}
		size_t get_h() const {
			return _h;
		}
		buffer_set_type get_type() const {
			return _type;
		}

		device_color *get_color_arr() const {
			return _carr;
		}
		mem_depth_buffer::element_type *get_depth_arr() const {
			return _dbuf.get_arr();
		}
		mem_stencil_buffer::element_type *get_stencil_arr() const {
			return _sbuf.get_arr();
		}

		template <typename T> T *get_at(size_t x, size_t y, T *ptr) const {
			return ptr + (_w * y + x);
		}

		void display(HDC dc) const {
#ifdef DEBUG
			if (_type != buffer_set_type::sys) {
				throw std::invalid_argument("wrong buffer type");
			}
#endif
			static_cast<const sys_color_buffer*>(_cbuf)->display(dc, _w, _h);
		}
	protected:
		size_t _w, _h;

		void *_cbuf;
		device_color *_carr;
		buffer_set_type _type = buffer_set_type::nograph;

		mem_depth_buffer _dbuf;
		mem_stencil_buffer _sbuf;
	};
}