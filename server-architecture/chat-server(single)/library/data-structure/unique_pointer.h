#pragma once
#include <malloc.h>
#include <utility>
#include <type_traits>

namespace data_structure {
	template<typename type>
	class unique_pointer final {
	public:
		inline constexpr explicit unique_pointer(void) noexcept
			: _value(nullptr) {
		}
		inline constexpr unique_pointer(nullptr_t) noexcept
			: _value(nullptr) {
		};
		inline unique_pointer(type* value) noexcept
			: _value(value) {
		}
		template<typename... argument>
		inline explicit unique_pointer(argument&&... arg) noexcept {
			_value = static_cast<type*>(malloc(sizeof(type)));
			if constexpr (std::is_class_v<type>) {
				if constexpr (std::is_constructible_v<type, argument...>)
					::new(reinterpret_cast<void*>(_value)) type(std::forward<argument>(arg)...);
			}
			else if constexpr (1 == sizeof...(arg))
#pragma warning(suppress: 6011)
				* _value = type(std::forward<argument>(arg)...);
		}
		inline unique_pointer(unique_pointer&) noexcept = delete;
		inline explicit unique_pointer(unique_pointer&& rhs) noexcept
			: _value(rhs._value) {
			rhs._value = nullptr;
		};
		inline auto operator=(unique_pointer const&) noexcept -> unique_pointer & = delete;
		inline auto operator=(unique_pointer&& rhs) noexcept -> unique_pointer& {
			_value = rhs._value;
			rhs._value = nullptr;
			return *this;
		};
		inline ~unique_pointer(void) noexcept {
			if (nullptr != _value) {
				if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
					_value->~type();
				free(_value);
			}
		}
	public:
		inline auto operator*(void) noexcept -> type& {
			return *_value;
		}
		inline auto operator->(void) noexcept -> type* {
			return _value;
		}
	public:
		inline void set(type* value) noexcept {
			_value = value;
		}
		inline void reset(void) noexcept {
			_value = nullptr;
		}
		inline auto get(void) const noexcept -> type* {
			return _value;
		}
		template <class type>
		friend inline bool operator==(unique_pointer<type> const& value, nullptr_t) noexcept {
			return value._value == nullptr;
		}
	private:
		type* _value;
	};


	template<typename type>
	class unique_pointer<type[]> final {
	private:
		using size_type = unsigned int;
	public:
		inline constexpr explicit unique_pointer(void) noexcept
			: _value(nullptr) {
		}
		inline constexpr unique_pointer(nullptr_t) noexcept
			: _value(nullptr) {
		};
		inline unique_pointer(type* value) noexcept
			: _value(value) {
		}
		inline unique_pointer(unique_pointer&) noexcept = delete;
		inline explicit unique_pointer(unique_pointer&& rhs) noexcept
			: _value(rhs._value) {
			rhs._value = nullptr;
		};
		inline auto operator=(unique_pointer const&) noexcept -> unique_pointer & = delete;
		inline auto operator=(unique_pointer&& rhs) noexcept -> unique_pointer& {
			_value = rhs._value;
			rhs._value = nullptr;
			return *this;
		};
		inline ~unique_pointer(void) noexcept {
			if (nullptr != _value) {
				if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
					_value->~type();
				free(_value);
			}
		}
	public:
		inline auto operator[](size_type const index) noexcept -> type& {
			return _value[index];
		}
	public:
		inline void set(type* value) noexcept {
			_value = value;
		}
		inline void reset(void) noexcept {
			_value = nullptr;
		}
		inline auto get(void) const noexcept -> type* {
			return _value;
		}
		template <class type>
		friend inline bool operator==(unique_pointer<type> const& value, nullptr_t) noexcept {
			return value._value == nullptr;
		}
	private:
		type* _value;
	};
}