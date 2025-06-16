#pragma once
#include "coordinate.h"
#include "library/bit_grid.h"

namespace a_star {
	class grid final {
	private:
		using size_type = unsigned int;
	public:
		inline explicit grid(size_type const width, size_type const height) noexcept
			: _width(width), _height(height), _bit_grid(width, height) {
		}
		inline ~grid(void) noexcept = default;
	public:
		inline void set_tile(coordinate const& position, bool const flag) {
			_bit_grid.set_bit(position._x, position._y, flag);
		}
		inline bool get_tile(coordinate const& position) const noexcept {
			if (0 > position._x || _width <= position._x ||
				0 > position._y || _height <= position._y)
				return true;
			return _bit_grid.get_bit(position._x, position._y);
		}
		inline auto get_width(void) const noexcept -> size_type {
			return _width;
		}
		inline auto get_height(void) const noexcept -> size_type {
			return _height;
		}
		inline void clear(void) noexcept {
			_bit_grid.clear();
		}
	public:
		size_type _width, _height;
		library::bit_grid<unsigned long long> _bit_grid;
	};
}