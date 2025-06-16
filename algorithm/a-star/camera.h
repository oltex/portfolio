#pragma once
#include <Windows.h>

class camera final {
public:
	inline explicit camera(void) noexcept = default;
	inline ~camera(void) noexcept = default;
public:
	inline void set_position(POINT const& position) noexcept {
		_position = position;
	};
	inline void move_position(POINT const& offset) noexcept {
		_position.x += offset.x;
		_position.y += offset.y;
	};
	inline void set_rect(POINT const& rect) noexcept {
		_rect = rect;
	};
	inline void zoom(POINT const& point, float const offset) noexcept {
		POINT pos = camera_to_client(point);
		_zoom += offset;
		if (_zoom < 0.1f)
			_zoom = 0.1f;
		POINT pos2 = camera_to_client(point);

		_position.x += pos.x - pos2.x;
		_position.y += pos.y - pos2.y;
	};
public:
	inline POINT clinet_to_camera(POINT const position)  const noexcept {
		POINT result;
		result.x = static_cast<LONG>((position.x - _position.x) / (float)_rect.x * _zoom * _rect.x + _rect.x * 0.5f);
		result.y = static_cast<LONG>((position.y - _position.y) / (float)_rect.y * _zoom * _rect.y + _rect.y * 0.5f);
		return result;
	};
	inline RECT clinet_to_camera(RECT const rect) const noexcept {
		RECT result;
		result.left = static_cast<LONG>((rect.left - _position.x) / (float)_rect.x * _zoom * _rect.x + _rect.x * 0.5f);
		result.right = static_cast<LONG>((rect.right - _position.x) / (float)_rect.x * _zoom * _rect.x + _rect.x * 0.5f);
		result.top = static_cast<LONG>((rect.top - _position.y) / (float)_rect.y * _zoom * _rect.y + _rect.y * 0.5f);
		result.bottom = static_cast<LONG>((rect.bottom - _position.y) / (float)_rect.y * _zoom * _rect.y + _rect.y * 0.5f);
		return result;
	};
	inline POINT camera_to_client(POINT const position) const noexcept {
		POINT result;
		result.x = static_cast<LONG>((position.x - _rect.x * 0.5f) / _rect.x / _zoom * _rect.x + _position.x);
		result.y = static_cast<LONG>((position.y - _rect.y * 0.5f) / _rect.y / _zoom * _rect.y + _position.y);
		return result;
	};
public:
	POINT _position{};
	POINT _rect{};
	float _zoom = 1.f;
};