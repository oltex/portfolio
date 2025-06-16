#pragma once
#include <initializer_list>
#include <utility>
#include <stdlib.h>

namespace data_structure {
	template<typename type>
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
			reserve(static_cast<size_type>(end - begin));
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
		//not implemented
		inline auto operator=(vector const& rhs) noexcept -> vector&;
		//not implemented
		inline auto operator=(vector&& rhs) noexcept -> vector&;
		inline ~vector(void) noexcept {
			clear();
			free(_array);
		}
	public:
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
			type* element = _array + _size++;
			if constexpr (std::is_class_v<type>) {
				if constexpr (std::is_constructible_v<type, argument...>)
					::new(reinterpret_cast<void*>(element)) type(std::forward<argument>(arg)...);
			}
			else if constexpr (1 == sizeof...(arg))
#pragma warning(suppress: 6011)
				* element = type(std::forward<argument>(arg)...);
			return *element;
		}
		inline void pop_back(void) noexcept {
			--_size;
			if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
				_array[_size].~type();
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
				_array = reinterpret_cast<type*>(realloc(_array, sizeof(type) * _capacity));
			}
		}
		template<typename... argument>
		inline void resize(size_type const size, argument&&... arg) noexcept {
			if (size > _capacity)
				reserve(size);

			if (size > _size) {
				if constexpr (std::is_class_v<type>) {
					if constexpr (std::is_constructible_v<type, argument...>)
						while (size != _size)
							new(_array + _size++) type(std::forward<argument>(arg)...);
					else
						_size = size;
				}
				else if constexpr (1 == sizeof...(arg))
					while (size != _size)
						_array[_size++] = type(std::forward<argument>(arg)...);
				else
					_size = size;
			}
			else {
				if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
					while (size != _size)
						pop_back();
				else
					_size = size;
			}
		}
		inline void swap(vector& rhs) noexcept {
			size_type size = _size;
			_size = rhs._size;
			rhs._size = size;

			size_type capacity = _capacity;
			_capacity = rhs._capacity;
			rhs._capacity = capacity;

			type* arr = _array;
			_array = rhs._array;
			rhs._array = arr;
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
			if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
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