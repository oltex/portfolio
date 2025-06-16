#pragma once
#include <windowsx.h>

#include "camera.h"
#include "tile.h"
#include "recode.h"
#include "test.h"

#include "jump-point-search/grid.h"
#include "jump-point-search/path.h"

class application final {
private:
	using size_type = unsigned int;
public:
	inline explicit application(HWND hwnd) noexcept
		: _hwnd(hwnd), _grid(200, 200), _path(_grid) {

		HDC hdc = GetDC(_hwnd);
		RECT rect;
		GetClientRect(_hwnd, &rect);
		_hdc = CreateCompatibleDC(hdc);
		_bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
		_brush = CreateSolidBrush(RGB(30, 30, 30));
		ReleaseDC(_hwnd, hdc);

		SelectObject(_hdc, _bitmap);
		_camera.set_position(POINT{ rect.right / 2, rect.bottom / 2 });
		_camera.set_rect(POINT{ rect.right, rect.bottom });
	}
public:
	inline void initialize(void) const noexcept {
		ShowWindow(_hwnd, true);
		UpdateWindow(_hwnd);
	}
	inline void procedure(HWND const hwnd, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept {
		switch (message) {
		case WM_LBUTTONDOWN: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			_tile._drag = true;
			_tile._erase = _grid.get_tile(jump_point_search::coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size });
			jump_point_search::coordinate coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size };
			if (_grid.in_bound(coordinate))
				_grid.set_tile(coordinate, !_tile._erase);
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_LBUTTONUP: {
			_tile._drag = false;
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_MBUTTONDOWN: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			_path.set_source(jump_point_search::coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size });
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_RBUTTONDOWN: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			_path.set_destination(jump_point_search::coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size });
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_KEYDOWN: {
			_test.set(wparam, _hwnd, _grid, _path);
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_TIMER: {
			if (1 != wparam)
				break;
			_test.run(_grid, _path);
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_MOUSEMOVE: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			if (false == _tile._drag)
				break;
			jump_point_search::coordinate coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size };
			if (_grid.in_bound(coordinate))
				_grid.set_tile(coordinate, !_tile._erase);
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_MOUSEWHEEL: {
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(_hwnd, &point);
			_camera.zoom(point, delta > 0 ? 0.1f : -0.1f);
			InvalidateRect(_hwnd, nullptr, true);
		} break;
		case WM_PAINT: {
			RECT rect;
			GetClientRect(_hwnd, &rect);
			FillRect(_hdc, &rect, (HBRUSH)_brush);

			_tile.paint(_hdc, _camera, _grid, _path);
			_recode.paint(_hdc, _path);

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(_hwnd, &ps);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, _hdc, 0, 0, SRCCOPY);
			EndPaint(_hwnd, &ps);
		} break;
		}
	}
public:
	HWND _hwnd;
	HDC _hdc;
	HGDIOBJ _bitmap;
	HGDIOBJ _brush;

	camera _camera;
	tile _tile;
	recode _recode;
	test _test;

	jump_point_search::grid _grid;
	jump_point_search::path _path;
};