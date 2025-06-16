#pragma once
#pragma comment(lib, "onecore.lib")
#include <malloc.h>
#include <utility>
#include <Windows.h>

namespace library::memory {
	inline auto allocate(size_t const size) noexcept -> void* {
		return reinterpret_cast<void*>(::malloc(size));
	}
	inline auto allocate(size_t const size, size_t const align) noexcept -> void* {
		return reinterpret_cast<void*>(::_aligned_malloc(size, align));
	}
	template<typename type>
		requires (!std::is_void_v<type>)
	inline auto allocate(void) noexcept -> type* {
		if constexpr (16 >= alignof(type))
			return reinterpret_cast<type*>(::malloc(sizeof(type)));
		else
			return reinterpret_cast<type*>(::_aligned_malloc(sizeof(type), alignof(type)));
	}
	template<typename type>
		requires (!std::is_void_v<type>)
	inline auto allocate(size_t const size) noexcept -> type* {
		if constexpr (16 >= alignof(type))
			return reinterpret_cast<type*>(::malloc(sizeof(type) * size));
		else
			return reinterpret_cast<type*>(::_aligned_malloc(sizeof(type) * size, alignof(type)));
	}

	inline auto reallocate(void* pointer, size_t const size) noexcept -> void* {
		return reinterpret_cast<void*>(realloc(pointer, size));
	}
	template<typename type>
		requires (!std::is_void_v<type>)
	inline auto reallocate(type* pointer, size_t const size) noexcept -> type* {
		if constexpr (16 >= alignof(type))
			return reinterpret_cast<type*>(::realloc(pointer, sizeof(type) * size));
		else
			return reinterpret_cast<type*>(::_aligned_realloc(pointer, sizeof(type) * size, alignof(type)));
	}

	inline auto deallocate(void* const pointer) noexcept {
		::free(pointer);
	}
	template<typename type>
		requires (!std::is_void_v<type>)
	inline auto deallocate(type* const pointer) noexcept {
		if constexpr (16 >= alignof(type))
			::free(pointer);
		else
			::_aligned_free(pointer);
	}

	template<typename type, typename... argument>
	inline auto construct(type& instance, argument&&... arg) noexcept {
		if constexpr (std::is_constructible_v<type, argument...>)
			if constexpr (std::is_class_v<type>)
				::new(reinterpret_cast<void*>(&instance)) type(std::forward<argument>(arg)...);
			else if constexpr (0 < sizeof...(arg))
#pragma warning(suppress: 6011)
				instance = type(std::forward<argument>(arg)...);
	}
	template<typename type>
	inline auto destruct(type& instance) noexcept {
		if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
			instance.~type();
	}
}
