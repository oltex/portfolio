#pragma once
#include "../../../data-structure/bit-grid/bit_grid.h"
#include "coordinate.h"

namespace algorithm::jump_point_search {
	class grid final {
	private:
		using size_type = unsigned int;
		using div_type = div_t;
	public:
		inline explicit grid(size_type const width, size_type const height) noexcept
			: _width(width), _height(height),
			_bit_grid{ library::data_structure::bit_grid<unsigned long long>(width, height), library::data_structure::bit_grid<unsigned long long>(height, width) } {
		};
		inline ~grid(void) noexcept = default;
	public:
		inline void set_tile(coordinate const& position, bool const flag) noexcept {
			_bit_grid[0].set_bit(position._x, position._y, flag);
			_bit_grid[1].set_bit(position._y, position._x, flag);
		}
		inline auto get_tile(coordinate const& position) const noexcept -> bool {
			return _bit_grid[0].get_bit(position._x, position._y);
		}
		inline auto get_width(void) const noexcept -> size_type {
			return _width;
		};
		inline auto get_height(void) const noexcept -> size_type {
			return _height;
		};
		inline auto get_div(coordinate position, axis const axi) const noexcept -> div_type {
			if (horizontal == axi)
				return _bit_grid[axi].get_div(position._x, position._y);
			return _bit_grid[axi].get_div(position._y, position._x);
		}
		inline auto get_div(coordinate position, axis const axi, movement const move) const noexcept -> div_type {
			if (horizontal == axi) {
				position._x = backward == move ? 0 : _width - 1;
				return _bit_grid[axi].get_div(position._x, position._y);
			}
			else {
				position._y = backward == move ? 0 : _height - 1;
				return _bit_grid[axi].get_div(position._y, position._x);
			}
		}
		inline auto get_word(size_type const index, axis const axis) const noexcept -> unsigned long long {
			return _bit_grid[axis].get_word(index);
		}
		inline auto get_position(div_t const& div, axis const axis) const noexcept -> coordinate {
			library::data_structure::pair<size_type, size_type> pair = _bit_grid[axis].get_pos(div);
			return coordinate(pair._first, pair._second);
		}
		inline bool in_bound(coordinate position) const noexcept {
			if (0 > position._x || _width <= position._x || 0 > position._y || _height <= position._y)
				return false;
			return true;
		}
		inline void clear(void) noexcept {
			_bit_grid[0].clear();
			_bit_grid[1].clear();
		}
	public:
		library::data_structure::bit_grid<unsigned long long> _bit_grid[2];
		size_type _width, _height;
	};
}