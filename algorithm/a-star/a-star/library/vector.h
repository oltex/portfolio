#pragma once
#include <initializer_list>
#include <utility>
#include <stdlib.h>
#include "memory.h"
#include "swap.h"

namespace library {
	template<typename type, bool placement = true>
	class vector {
	public:
		using iterator = type*;
	private:
		using size_type = unsigned int;
	public:
		inline explicit vector(void) noexcept = default;
		inline explicit vector(std::initializer_list<type> const& list) noexcept {
			reserve(list.size());
			for (auto& iter : list)
				emplace_back(iter);
		}
		inline explicit vector(iterator const& begin, iterator const& end) noexcept {
			reserve(end - begin);
			for (auto iter = begin; iter != end; ++iter)
				emplace_back(*iter);
		}
		inline explicit vector(vector const& rhs) noexcept
			: vector(rhs.begin(), rhs.end()) {
		}
		inline explicit vector(vector&& rhs) noexcept
			: _array(rhs._array), _size(rhs._size), _capacity(rhs._capacity) {
			rhs._array = nullptr;
			rhs._size = 0;
			rhs._capacity = 0;
		}
		inline auto operator=(vector const& rhs) noexcept -> vector&;
		inline auto operator=(vector&& rhs) noexcept -> vector&;
		inline ~vector(void) noexcept {
			clear();
			memory::deallocate<type>(_array);
		}

		template<typename universal>
		inline void push_back(universal&& value) noexcept {
			emplace_back(std::forward<universal>(value));
		}
		template<typename... argument>
		inline auto emplace_back(argument&&... arg) noexcept -> type& {
			if (_size >= _capacity) {
				size_type capacity = static_cast<size_type>(_capacity * 1.5f);
				if (_size >= capacity)
					capacity++;
				reserve(capacity);
			}
			type& element = _array[_size++];
			if (true == placement)
				memory::construct(element, std::forward<argument>(arg)...);
			return element;
		}
		inline void pop_back(void) noexcept {
			--_size;
			if (true == placement)
				memory::destruct(_array[_size]);
		}
	public:
		inline auto front(void) const noexcept ->type& {
			return _array[0];
		}
		inline auto back(void) const noexcept ->type& {
			return _array[_size - 1];
		}
		inline auto operator[](size_type const index) const noexcept ->type& {
			return _array[index];
		}
		inline auto begin(void) const noexcept -> iterator {
			return _array;
		}
		inline auto end(void) const noexcept -> iterator {
			return _array + _size;
		}
		inline auto data(void) noexcept -> type* {
			return _array;
		}
	public:
		inline void reserve(size_type const capacity) noexcept {
			if (_size <= capacity) {
				_capacity = capacity;
#pragma warning(suppress: 6308)
				_array = memory::reallocate<type>(_array, _capacity);
			}
		}
		template<typename... argument>
		inline void resize(size_type const size, argument&&... arg) noexcept {
			if (size > _capacity)
				reserve(size);

			if constexpr (true == placement) {
				if (size > _size)
					if constexpr (std::is_constructible_v<type, argument...>)
						while (size != _size)
							memory::construct(_array[_size++], std::forward<argument>(arg)...);
					else
						if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
							while (size != _size)
								pop_back();
			}
			_size = size;
		}
		inline void swap(vector& rhs) noexcept {
			library::swap(_size, rhs._size);
			library::swap(_capacity, rhs._capacity);
			library::swap(_array, rhs._array);
		}
		//not implemented
		inline void assign(size_type const size, type const& value) noexcept {
			clear();
			if (_capacity < size)
				reserve(size);
			for (; _size < size; ++_size) {
				if constexpr (std::is_class_v<type>)
					new(_array + _size) type(value);
				else
					_array[_size] = value;
			}
		}
		inline void clear(void) noexcept {
			if constexpr (true == placement && std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
				while (0 != _size)
					pop_back();
			else
				_size = 0;
		}
	public:
		inline auto size(void) const noexcept -> size_type {
			return _size;
		}
		inline bool empty(void) const noexcept {
			return 0 == _size;
		}
		inline auto capacity(void) const noexcept -> size_type {
			return _capacity;
		}
	private:
		size_type _size = 0;
		size_type _capacity = 0;
		type* _array = nullptr;
	};
}