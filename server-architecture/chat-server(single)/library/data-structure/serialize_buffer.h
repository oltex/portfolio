#pragma once
//#define debug
#include <memory>
#include <string>
#include <string_view>

namespace data_structure {
	template<typename type>
	concept string_size = std::_Is_any_of_v<type, unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long>;

	template<size_t _capacity = 2048>
	class serialize_buffer {
	private:
		using byte = unsigned char;
		using size_type = unsigned int;
		using iterator = byte*;
	public:
		inline explicit serialize_buffer(void) noexcept = default;
		inline explicit serialize_buffer(serialize_buffer const& rhs) noexcept = default;
		inline explicit serialize_buffer(serialize_buffer&& rhs) noexcept = default;
		inline auto operator=(serialize_buffer const& rhs) noexcept -> serialize_buffer&;
		inline auto operator=(serialize_buffer&& rhs) noexcept -> serialize_buffer&;
		inline ~serialize_buffer(void) noexcept = default;
	public:
		template<typename type>
		inline auto operator<<(type const& value) noexcept -> serialize_buffer& requires std::is_arithmetic_v<type> {
#ifdef debug
			if (sizeof(type) + _rear > _capacity) {
				_fail = true;
				return *this;
			}
#endif
			reinterpret_cast<type&>(_array[_rear]) = value;
			_rear += sizeof(type);
			return *this;
		}
		inline void push(byte* const buffer, size_type const length) noexcept {
#ifdef debug
			if (length + _rear > _capacity) {
				_fail = true;
				return;
			}
#endif
			memcpy(_array + _rear, buffer, length);
			_rear += length;
		}
		template<typename type>
		inline auto operator>>(type& value) noexcept -> serialize_buffer& requires std::is_arithmetic_v<type> {
#ifdef debug
			if (sizeof(type) + _front > _rear) {
				_fail = true;
				return *this;
			}
#endif
			value = reinterpret_cast<type&>(_array[_front]);
			_front += sizeof(type);
			return *this;
		}
		inline void peek(byte* const buffer, size_type const length) const noexcept {
#ifdef debug
			if (length + _front > _rear) {
				_fail = true;
				return;
			}
#endif
			memcpy(buffer, _array + _front, length);
		}
		inline void pop(size_type const length) noexcept {
			_front += length;
		}
		inline auto begin(void) noexcept -> iterator {
			return _array + _front;
		}
		inline auto end(void) noexcept -> iterator {
			return _array + _rear;
		}
#ifdef debug
		inline operator bool(void) const noexcept {
			return !_fail;
		}
		inline auto fail(void) const noexcept {
			return _fail;
		}
#endif
		inline void clear(void) noexcept {
			_front = _rear = 0;
#ifdef debug
			_fail = false;
#endif
		}
		inline auto size(void) const noexcept -> size_type {
			return _rear - _front;
		}
		inline auto front(void) const noexcept -> size_type {
			return _front;
		}
		inline auto rear(void) const noexcept -> size_type {
			return _rear;
		}
		inline static auto capacity(void) noexcept -> size_type {
			return _capacity;
		}
		inline void move_front(size_type const length) noexcept {
			_front += length;
		}
		inline void move_rear(size_type const length) noexcept {
			_rear += length;
		}
		inline auto data(void) noexcept -> byte* {
			return _array;
		}
	protected:
		size_type _front = 0;
		size_type _rear = 0;
#ifdef debug
		bool _fail = false;
#endif
		byte _array[_capacity];
	};
}