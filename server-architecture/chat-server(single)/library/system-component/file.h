#pragma once
#include "handle.h"
#include <string_view>
#include <Windows.h>

namespace system_component {
	class file final : public handle {
	public:
		inline explicit file(void) noexcept = default;
		inline explicit file(std::wstring_view path, unsigned long desired_access, unsigned long share_mode, unsigned long creation_disposition, unsigned long flags_and_attributes) noexcept
			: handle(CreateFileW(path.data(), desired_access, share_mode, nullptr, creation_disposition, flags_and_attributes, nullptr)) {
		};
		inline explicit file(file const&) noexcept = delete;
		inline explicit file(file&& rhs) noexcept
			: handle(std::move(rhs)) {
		};
		inline auto operator=(file const&) noexcept -> file & = delete;
		inline auto operator=(file&& rhs) noexcept -> file& {
			handle::operator=(std::move(rhs));
			return *this;
		};
		inline virtual ~file(void) noexcept override = default;

		inline void create(std::wstring_view path, unsigned long desired_access, unsigned long share_mode, unsigned long  creation_disposition, unsigned long flags_and_attributes) noexcept {
			_handle = CreateFileW(path.data(), desired_access, share_mode, nullptr, creation_disposition, flags_and_attributes, nullptr);
		}
		inline bool read(void* const buffer, unsigned long const length) const noexcept {
			return ReadFile(_handle, buffer, length, nullptr, nullptr);
		}
		inline bool write(void const* const buffer, unsigned long const length) const noexcept {
			return WriteFile(_handle, buffer, length, nullptr, nullptr);
		}
		inline auto set_pointer(long const distance_to_move, unsigned long move_method) const noexcept -> unsigned long {
			return SetFilePointer(_handle, distance_to_move, nullptr, move_method);
		}
		inline auto set_pointer_ex(LARGE_INTEGER const distance_to_move, unsigned long move_method) const noexcept -> bool {
			return SetFilePointerEx(_handle, distance_to_move, nullptr, move_method);
		}
		inline void set_end(void) const noexcept {
			SetEndOfFile(_handle);
		}
		inline void flush_buffer(void) const noexcept {
			FlushFileBuffers(_handle);
		}
		inline auto get_size_ex(void) const noexcept {
			LARGE_INTEGER size;
			GetFileSizeEx(_handle, &size);
			return size;
		}

		inline static auto get_attribute(std::wstring_view const path) noexcept -> unsigned long {
			return GetFileAttributesW(path.data());
		}
		inline static bool create_directory(std::wstring_view const path) noexcept {
			return CreateDirectoryW(path.data(), nullptr);
		}
	};
}