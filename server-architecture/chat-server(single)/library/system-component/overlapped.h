#pragma once
//#include "../multi/event.h"
#include <Windows.h>

namespace system_component {
	class overlapped final {
	public:
		inline explicit overlapped(void) noexcept = default;
		inline explicit overlapped(overlapped const&) noexcept = delete;
		inline explicit overlapped(overlapped&&) noexcept = delete;
		inline auto operator=(overlapped const&) noexcept -> overlapped & = delete;
		inline auto operator=(overlapped&&) noexcept -> overlapped & = delete;
		inline ~overlapped(void) noexcept = default;
	public:
		//inline void set_event(multi::event& event) noexcept {
		//	_overlapped.hEvent = event.data();
		//}
		inline auto get_result(HANDLE handle, unsigned long* byte, bool const wait) noexcept -> bool {
			return GetOverlappedResult(handle, &_overlapped, byte, wait);
		}
		inline bool has_completed(void) const noexcept {
			return HasOverlappedIoCompleted(&_overlapped);
		}
	public:
		inline void clear(void) noexcept {
			memset(&_overlapped, 0, sizeof(_OVERLAPPED));
		}
		inline auto data(void) noexcept -> _OVERLAPPED& {
			return _overlapped;
		}
	private:
		_OVERLAPPED _overlapped;
	};
}