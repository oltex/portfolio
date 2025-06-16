#pragma once
#include <windows.h>

namespace system_component::lock {
	class critical_section final {
	public:
		inline explicit critical_section(void) noexcept {
			InitializeCriticalSection(&_critical_section);
		};
		inline explicit critical_section(critical_section const&) noexcept = delete;
		inline explicit critical_section(critical_section&&) noexcept = delete;
		inline auto operator=(critical_section const&) noexcept -> critical_section & = delete;
		inline auto operator=(critical_section&&) noexcept -> critical_section & = delete;
		inline ~critical_section(void) noexcept {
			DeleteCriticalSection(&_critical_section);
		};
	public:
		inline void enter(void) noexcept {
			EnterCriticalSection(&_critical_section);
		}
		inline bool try_enter(void) noexcept {
			return TryEnterCriticalSection(&_critical_section);
		}
		inline void leave(void) noexcept {
			LeaveCriticalSection(&_critical_section);
		}
	public:
		inline auto data(void) noexcept -> CRITICAL_SECTION& {
			return _critical_section;
		}
	private:
		CRITICAL_SECTION _critical_section;
	};
}