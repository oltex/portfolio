#pragma once
#include "../../design-pettern/singleton/singleton.h"
#include "../../system/window/window.h"
#include "../../system/window/device_context.h"
#include <windowsx.h>

#include "camera.h"
#include "tile.h"
#include "recode.h"
#include "test.h"

#include "jump-point-search/grid.h"
#include "jump-point-search/path.h"

class client final : public library::design_pattern::singleton<client, library::design_pattern::member_static<client>> {
private:
	friend class library::design_pattern::singleton<client, library::design_pattern::member_static<client>>;
	using size_type = unsigned int;
public:
	inline static auto constructor(window::window&& window) noexcept -> client& {
		_instance = new client(std::move(window));
		atexit(destructor);
		return *_instance;
	}
private:
	inline explicit client(window::window&& window) noexcept
		: _window(std::move(window)),
		_dc(_window.get_device_context().create_compatible_device_context()),
		_bitmap(_window.get_device_context().create_compatible_bitmap(_window.get_client_rect().right, _window.get_client_rect().bottom)),
		_background(RGB(30, 30, 30)),
		_grid(200, 200), _path(_grid) {

		_dc.select_object(_bitmap);
		RECT rect = _window.get_client_rect();
		_camera.set_position(POINT{ rect.right / 2, rect.bottom / 2 });
		_camera.set_rect(POINT{ rect.right, rect.bottom });
	}
	inline explicit client(client const& rhs) noexcept = delete;
	inline explicit client(client&& rhs) noexcept = delete;
	inline auto operator=(client const& rhs) noexcept -> client & = delete;
	inline auto operator=(client&& rhs) noexcept -> client & = delete;
	inline ~client(void) noexcept = default;
public:
	inline void initialize(void) const noexcept {
		_window.show(true);
		_window.update();
	}
	inline void procedure(HWND const hwnd, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept {
		switch (message) {
		case WM_LBUTTONDOWN: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			_tile._drag = true;
			_tile._erase = _grid.get_tile(algorithm::jump_point_search::coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size });
			algorithm::jump_point_search::coordinate coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size };
			if (_grid.in_bound(coordinate))
				_grid.set_tile(coordinate, !_tile._erase);
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_LBUTTONUP: {
			_tile._drag = false;
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_MBUTTONDOWN: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			_path.set_source(algorithm::jump_point_search::coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size });
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_RBUTTONDOWN: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			_path.set_destination(algorithm::jump_point_search::coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size });
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_KEYDOWN: {
			_test.set(wparam, _window, _grid, _path);
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_TIMER: {
			if (1 != wparam)
				break;
			_test.run(_grid, _path);
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_MOUSEMOVE: {
			POINT point = _camera.camera_to_client(POINT(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
			if (false == _tile._drag)
				break;
			algorithm::jump_point_search::coordinate coordinate{ (unsigned int)point.x / _tile._size, (unsigned int)point.y / _tile._size };
			if (_grid.in_bound(coordinate))
				_grid.set_tile(coordinate, !_tile._erase);
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_MOUSEWHEEL: {
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);
			POINT point;
			GetCursorPos(&point);
			_window.screen_to_client(&point);
			if (delta > 0)
				_camera.zoom(point, 0.1f);
			else
				_camera.zoom(point, -0.1f);
			_window.invalidate_rect(nullptr, true);
		} break;
		case WM_PAINT: {
			RECT rect = _window.get_client_rect();
			_dc.fill_rect(&rect, _background);

			_tile.paint(_dc, _camera, _grid, _path);
			_recode.paint(_dc, _path);

			window::device_context dc = _window.begin_paint();
			dc.bit_blt(0, 0, rect.right, rect.bottom, _dc, 0, 0, SRCCOPY);
			_window.end_paint(dc);
		} break;
		}
	}
public:
	window::window _window;
	window::device_context _dc;
	window::bitmap _bitmap;
	window::brush _background;

	camera _camera;
	tile _tile;
	recode _recode;
	test _test;

	algorithm::jump_point_search::grid _grid;
	algorithm::jump_point_search::path _path;
};