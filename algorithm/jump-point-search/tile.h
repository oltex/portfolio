#pragma once
#include "camera.h"
#include "jump-point-search/grid.h"
#include "jump-point-search/path.h"

#include <string>
#include <sstream>
#include <iomanip>

class tile final {
private:
	using size_type = unsigned int;
public:
	inline explicit tile(void) noexcept {
		_line = CreatePen(PS_SOLID, 2, RGB(10, 10, 10));
		_wall = CreateSolidBrush(RGB(120, 120, 120));
		_start = CreateSolidBrush(RGB(0, 255, 0));
		_end = CreateSolidBrush(RGB(255, 0, 0));
		_open = CreateSolidBrush(RGB(255, 255, 0));
		_close = CreateSolidBrush(RGB(0, 255, 255));
		_path = CreateSolidBrush(RGB(0, 0, 255));
		_info = CreateFontW(12, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
			L"Malgun Gothic");
		_parent = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	}
	inline ~tile(void) noexcept = default;
public:
	inline void paint(HDC hdc, camera& camera, jump_point_search::grid& grid, jump_point_search::path& path) noexcept {
		SelectObject(hdc, _line);
		auto width = grid.get_width();
		auto height = grid.get_height();
		for (size_type i = 0; i < width + 1; ++i) {
			POINT move_pos = camera.clinet_to_camera(POINT(i * _size, 0));
			POINT line_pos = camera.clinet_to_camera(POINT(i * _size, height * _size));
			MoveToEx(hdc, move_pos.x, move_pos.y, nullptr);
			LineTo(hdc, line_pos.x, line_pos.y);
		}
		for (size_type i = 0; i < height + 1; ++i) {
			POINT move_pos = camera.clinet_to_camera(POINT(0, i * _size));
			POINT line_pos = camera.clinet_to_camera(POINT(width * _size, i * _size));
			MoveToEx(hdc, move_pos.x, move_pos.y, nullptr);
			LineTo(hdc, line_pos.x, line_pos.y);
		}

		SelectObject(hdc, GetStockObject(NULL_PEN));
		SelectObject(hdc, _wall);
		for (size_type i = 0; i < height; ++i) {
			for (size_type j = 0; j < width; ++j) {
				if (!grid.get_tile(jump_point_search::coordinate(j, i)))
					continue;
				RECT rect(j * _size, i * _size, (j + 1) * _size, (i + 1) * _size);
				rect = camera.clinet_to_camera(rect);
				Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
			}
		}

		SelectObject(hdc, _close);
		for (auto iter = path._parent.begin(); iter != path._parent.end();) {
			if (!(*iter).expired()) {
				jump_point_search::coordinate position = (*iter)->_position;
				RECT rect(position._x * _size, position._y * _size, (position._x + 1) * _size, (position._y + 1) * _size);
				rect = camera.clinet_to_camera(rect);
				Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
				++iter;
			}
			else
				iter = path._parent.erase(iter);
		}
		SelectObject(hdc, _open);
		for (auto& iter : path._heap._vector) {
			jump_point_search::coordinate position = iter->_node->_position;
			RECT rect = camera.clinet_to_camera(RECT(position._x * _size, position._y * _size, (position._x + 1) * _size, (position._y + 1) * _size));
			Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		}

		SelectObject(hdc, _path);
		for (auto& iter : path.result()) {
			RECT rect = camera.clinet_to_camera(RECT(iter._x * _size, iter._y * _size, (iter._x + 1) * _size, (iter._y + 1) * _size));
			Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		}

		{
			SelectObject(hdc, _start);
			RECT rect = camera.clinet_to_camera(RECT(path._source._x * _size, path._source._y * _size, (path._source._x + 1) * _size, (path._source._y + 1) * _size));
			Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		}
		{
			SelectObject(hdc, _end);
			RECT rect = camera.clinet_to_camera(RECT(path._destination._x * _size, path._destination._y * _size, (path._destination._x + 1) * _size, (path._destination._y + 1) * _size));
			Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		}

		SelectObject(hdc, _info);
		SetTextColor(hdc, RGB(0, 0, 0));
		SelectObject(hdc, _parent);
		for (auto iter = path._parent.begin(); iter != path._parent.end();) {
			if (!(*iter).expired()) {
				jump_point_search::coordinate position = (*iter)->_position;
				POINT point = camera.clinet_to_camera(POINT(position._x * _size + _size / 2, position._y * _size + _size / 2));
				RECT rect = camera.clinet_to_camera(RECT(position._x * _size, position._y * _size, (position._x + 1) * _size, (position._y + 1) * _size));

				if ((*iter)->_parent != nullptr) {
					size_type x = point.x;
					size_type y = point.y;
					if ((*iter)->_parent->_position._x < position._x)
						x = rect.left;
					else if ((*iter)->_parent->_position._x > position._x)
						x = rect.right;
					if ((*iter)->_parent->_position._y < position._y)
						y = rect.top;
					else if ((*iter)->_parent->_position._y > position._y)
						y = rect.bottom;

					MoveToEx(hdc, point.x, point.y, nullptr);
					LineTo(hdc, x, y);
				}

				if (camera._zoom > 2.5f) {
					std::wstringstream full, ground, heuri;
					full << std::fixed << std::setprecision(2) << (*iter)->_full;
					ground << std::fixed << std::setprecision(2) << (*iter)->_ground;
					heuri << std::fixed << std::setprecision(2) << (*iter)->_full - (*iter)->_ground;
					std::wstring info = L"F: " + full.str() + L"\n" + L"G: " + ground.str() + L"\n" + L"H: " + heuri.str() + L"\n";
					DrawTextW(hdc, info.c_str(), (int)info.size(), &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);
				}
				++iter;
			}
			else
				iter = path._parent.erase(iter);
		}
		for (auto& iter : path._heap._vector) {
			jump_point_search::coordinate position = iter->_node->_position;
			POINT point = camera.clinet_to_camera(POINT(position._x * _size + _size / 2, position._y * _size + _size / 2));
			RECT rect = camera.clinet_to_camera(RECT(position._x * _size, position._y * _size, (position._x + 1) * _size, (position._y + 1) * _size));

			if (iter->_node->_parent != nullptr) {
				size_type x = point.x;
				size_type y = point.y;
				if (iter->_node->_parent->_position._x < position._x)
					x = rect.left;
				else if (iter->_node->_parent->_position._x > position._x)
					x = rect.right;
				if (iter->_node->_parent->_position._y < position._y)
					y = rect.top;
				else if (iter->_node->_parent->_position._y > position._y)
					y = rect.bottom;

				MoveToEx(hdc, point.x, point.y, nullptr);
				LineTo(hdc, x, y);
			}

			if (camera._zoom > 2.5f) {
				std::wstringstream full, ground, heuri;
				full << std::fixed << std::setprecision(2) << iter->_node->_full;
				ground << std::fixed << std::setprecision(2) << iter->_node->_ground;
				heuri << std::fixed << std::setprecision(2) << iter->_node->_full - iter->_node->_ground;
				std::wstring info = L"F: " + full.str() + L"\n" + L"G: " + ground.str() + L"\n" + L"H: " + heuri.str() + L"\n";
				DrawTextW(hdc, info.c_str(), (int)info.size(), &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);
			}
		}
	}
public:
	HGDIOBJ _line;
	HGDIOBJ _wall;
	HGDIOBJ _start;
	HGDIOBJ _end;
	HGDIOBJ _open;
	HGDIOBJ _close;
	HGDIOBJ _path;
	HGDIOBJ _info;
	HGDIOBJ _parent;

	int _size = 16;
	bool _drag = false;
	bool _erase = false;
};