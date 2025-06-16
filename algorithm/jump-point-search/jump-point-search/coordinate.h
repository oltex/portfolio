#pragma once
#include <stdlib.h>
#include <math.h>

namespace algorithm::jump_point_search {
	struct coordinate {
	private:
		using size_type = unsigned int;
	public:
		inline auto distance_manhattan(coordinate const& rhs) const noexcept -> size_type {
			size_type x = abs((int)_x - (int)rhs._x);
			size_type y = abs((int)_y - (int)rhs._y);
			return x + y;
		}
		inline auto distance_euclidean(coordinate const& rhs) const noexcept -> float {
			size_type x = abs((int)_x - (int)rhs._x);
			size_type y = abs((int)_y - (int)rhs._y);
			x = x * x;
			y = y * y;
			return sqrtf(static_cast<float>(x + y));
		}
		inline bool operator ==(coordinate const& rhs) const noexcept {
			return (_x == rhs._x && _y == rhs._y);
		}
		size_type _x, _y;
	};

	enum direction : unsigned char {
		up, up_right, right, right_down, down, down_left, left, left_up, all
	};
	using bit_direction = unsigned char;

	enum axis : bool {
		horizontal, vertical
	};
	enum movement : bool {
		backward, forward
	};
}