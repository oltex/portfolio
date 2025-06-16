#pragma once
#include "vector.h"
#include "pair.h"
#include <memory>

namespace library {
	template<typename type>
	class bit_grid final {
	private:
		using size_type = unsigned int;
		inline static unsigned char constexpr _bit = static_cast<size_type>(8 * sizeof(type));
		using div_type = div_t;
	public:
		inline explicit bit_grid(void) noexcept
			: _width(0), _height(0) {
		}
		inline explicit bit_grid(size_type const width, size_type const height) noexcept
			: _width(width), _height(height) {
			div_type _div = div(static_cast<size_type>(width * height), _bit);
			if (_div.rem)
				++_div.quot;
			_vector.assign(_div.quot, 0);
		}
		inline explicit bit_grid(bit_grid const& rhs) noexcept;
		inline explicit bit_grid(bit_grid&& rhs) noexcept;
		inline auto operator=(bit_grid const& rhs) noexcept -> bit_grid&;
		inline auto operator=(bit_grid&& rhs) noexcept -> bit_grid&;
		inline ~bit_grid(void) noexcept = default;
	public:
		inline void set_size(size_type const width, size_type const height) noexcept {
			_width = width;
			_height = height;
			div_type _div = div(static_cast<size_type>(width * height), _bit);
			if (_div.rem)
				++_div.quot;
			_vector.assign(_div.quot, 0);
		}
		inline void set_bit(size_type const x, size_type const y, bool const flag) noexcept {
			div_type _div = get_div(x, y);
			switch (flag) {
			case 0:
				_vector[_div.quot] &= ~(static_cast<type>(1) << _div.rem);
				break;
			case 1:
				_vector[_div.quot] |= static_cast<type>(1) << _div.rem;
				break;
			}
		}
		inline bool get_bit(size_type const x, size_type const y) const noexcept {
			div_type _div = get_div(x, y);
			return _vector[_div.quot] & static_cast<type>(1) << _div.rem;
		}
		inline auto get_word(size_type const x, size_type const y) const noexcept -> type {
			div_type _div = get_div(x, y);
			return _vector[_div.quot];
		}
		inline auto get_word(size_type const idx) const noexcept -> type {
			return _vector[idx];
		}
		inline auto get_div(size_type const x, size_type const y) const noexcept -> div_type {
			size_type index = x + _width * y;
			return div(index, _bit);
		}
		inline auto get_pos(div_type const& _div) const noexcept -> pair<size_type, size_type> {
			size_type index = _div.quot * _bit + _div.rem;
			return pair<size_type, size_type>{index% _width, index / _width};
		}
	public:
		inline void clear(void) noexcept {
			memset(_vector.data(), 0, sizeof(type) * _vector.capacity());
		}
		inline auto get_width(void) const noexcept -> size_type {
			return _width;
		}
		inline auto get_height(void) const noexcept -> size_type {
			return _height;
		}
		inline bool in_bound(size_type const x, size_type const y) const noexcept {
			if (x >= _width || y >= _height)
				return false;
			return true;
		}
	private:
		size_type _width, _height;
		vector<type> _vector;
	};
}