#pragma once
#include <type_traits>

namespace design_pattern::_thread_local {
	template<typename type>
	class singleton {
	protected:
		inline explicit singleton(void) noexcept = default;
		inline ~singleton(void) noexcept = default;
	private:
		inline explicit singleton(singleton const&) noexcept = delete;
		inline explicit singleton(singleton&&) noexcept = delete;
		inline auto operator=(singleton const&) noexcept -> singleton & = delete;
		inline auto operator=(singleton&&) noexcept -> singleton & = delete;
	public:
		inline static auto instance(void) noexcept -> type& {
			thread_local type _instance;
			return _instance;
		}
	};
}