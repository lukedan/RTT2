#pragma once

#include <Windows.h>
#include <Windowsx.h>
#include <string>

namespace rtt2 {
	class window {
	public:
		window(const std::string &cls_name, WNDPROC msg_handler) {
			HINSTANCE hinst = GetModuleHandle(nullptr);
			WNDCLASSEX wcex;
			std::memset(&wcex, 0, sizeof(wcex));
			wcex.style = CS_OWNDC;
			wcex.lpfnWndProc = msg_handler;
			wcex.cbSize = sizeof(wcex);
			wcex.hInstance = hinst;
			wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wcex.lpszClassName = cls_name.c_str();
			_wndatom = RegisterClassEx(&wcex);
			_hwnd = CreateWindowEx(
				0, cls_name.c_str(), cls_name.c_str(), WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				nullptr, nullptr, hinst, nullptr
			);
		}
		~window() {
			DestroyWindow(_hwnd);
			UnregisterClass(reinterpret_cast<LPCTSTR>(_wndatom), GetModuleHandle(nullptr));
		}

		bool idle() {
			MSG msg;
			if (PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				return true;
			}
			return false;
		}

		void set_center() {
			RECT r, work;
			GetWindowRect(_hwnd, &r);
			SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
			MoveWindow(
				_hwnd,
				(work.right - work.left - r.right + r.left) / 2 + work.left,
				(work.bottom - work.top - r.bottom + r.top) / 2 + work.top,
				r.right - r.left,
				r.bottom - r.top,
				false
			);
		}
		void set_client_size(size_t w, size_t h) {
			RECT r, c;
			GetWindowRect(_hwnd, &r);
			GetClientRect(_hwnd, &c);
			MoveWindow(
				_hwnd,
				r.left,
				r.top,
				w - c.right + r.right - r.left,
				h - c.bottom + r.bottom - r.top,
				false
			);
		}
		void set_caption(const std::string &str) {
			SetWindowText(_hwnd, str.c_str());
		}

		void get_center_pos(int &x, int &y) const {
			RECT r;
			GetWindowRect(_hwnd, &r);
			x = (r.left + r.right) / 2;
			y = (r.top + r.bottom) / 2;
		}
		void put_mouse_to_center() {
			int x, y;
			get_center_pos(x, y);
			set_mouse_global_pos(x, y);
		}

		void show() {
			ShowWindow(_hwnd, SW_SHOW);
		}
		void hide() {
			ShowWindow(_hwnd, SW_HIDE);
		}

		HWND get_handle() const {
			return _hwnd;
		}
		HDC get_dc() const {
			return GetDC(_hwnd);
		}
	protected:
		HWND _hwnd;
		ATOM _wndatom;
	};

	inline void get_xy_from_lparam(LPARAM lparam, SHORT &x, SHORT &y) {
		x = GET_X_LPARAM(lparam);
		y = GET_Y_LPARAM(lparam);
	}
}
