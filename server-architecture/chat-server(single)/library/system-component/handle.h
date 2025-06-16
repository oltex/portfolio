#pragma once
#include "overlapped.h"
#include <Windows.h>
#include <concepts>

namespace system_component {
	class handle {
	public:
		inline explicit handle(void) noexcept
			: _handle(INVALID_HANDLE_VALUE) {
		};
		inline explicit handle(HANDLE const handle_) noexcept
			: _handle(handle_) {
		};
		inline explicit handle(handle const&) noexcept = delete;
		inline explicit handle(handle&& rhs) noexcept
			: _handle(rhs._handle) {
			rhs._handle = INVALID_HANDLE_VALUE;
		};
		inline auto operator=(handle const&) noexcept -> handle & = delete;
		inline auto operator=(handle&& rhs) noexcept -> handle& {
			CloseHandle(_handle);
			_handle = rhs._handle;
			rhs._handle = INVALID_HANDLE_VALUE;
			return *this;
		};
		inline virtual ~handle(void) noexcept {
			CloseHandle(_handle);
		};

		inline void close(void) noexcept {
			CloseHandle(_handle);
			_handle = INVALID_HANDLE_VALUE;
		}
		inline void cancel_io(void) const noexcept {
			CancelIo(_handle);
		}
		inline void cancel_io_ex(void) const noexcept {
			CancelIoEx(_handle, nullptr);
		}
		inline void cancel_io_ex(overlapped overlapped) const noexcept {
			CancelIoEx(_handle, &overlapped.data());
		}
		inline auto wait_for_single(unsigned long const milli_second) noexcept -> unsigned long {
			return WaitForSingleObject(_handle, milli_second);
		}
		inline auto wait_for_single_ex(unsigned long const milli_second, bool alertable) noexcept -> unsigned long {
			return WaitForSingleObjectEx(_handle, milli_second, alertable);
		}
		inline auto data(void) noexcept -> HANDLE& {
			return _handle;
		}

		inline static auto wait_for_multiple(unsigned long const count, HANDLE* handle, bool const wait_all, unsigned long const milli_second) noexcept -> unsigned long {
			return WaitForMultipleObjects(count, handle, wait_all, milli_second);
		}
		template<std::derived_from<handle>... handle_type>
		inline static auto wait_for_multiple(bool const wait_all, unsigned long const milli_second, handle_type&... handle_) noexcept -> unsigned long {
			HANDLE handle_array[] = { handle_.data()... };
			return WaitForMultipleObjects(sizeof...(handle_), handle_array, wait_all, milli_second);
		}
		inline static auto wait_for_multiple_ex(unsigned long const count, HANDLE* handle, bool const wait_all, unsigned long const milli_second, bool alertable) noexcept {
			return WaitForMultipleObjectsEx(count, handle, wait_all, milli_second, alertable);
		}
	protected:
		HANDLE _handle;
	};
}