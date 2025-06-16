#pragma once
#include "../design-pattern/singleton.h"
#include "../system-component/lock/critical_section.h"
#include "../system-component/file.h"
#include "../system-component/time/unix.h"
#include "../system-component/time/date.h"
#include <string_view>
#include <chrono>
#include <iostream>
#include <Windows.h>
#include <unordered_map>

namespace utility {
	class logger final : public design_pattern::singleton<logger> {
	public:
		enum output : unsigned char { console = 0x01, file = 0x02 };
		enum class level : unsigned char { trace, debug, info, warning, error, fatal };
	private:
		friend class design_pattern::singleton<logger>;
		using flag = unsigned char;
		using size_type = unsigned int;
		inline static unsigned char constexpr none = static_cast<unsigned char>(level::fatal) + 1;
		inline static constexpr wchar_t const* const lm[] = { L"trace", L"debug", L"info", L"warning", L"error", L"fatal" };
		struct logged final {
			inline explicit logged(void) noexcept = default;
			inline explicit logged(logged const& rhs) noexcept = delete;
			inline explicit logged(logged&& rhs) noexcept = delete;
			inline auto operator=(logged const& rhs) noexcept -> logged & = delete;
			inline auto operator=(logged&& rhs) noexcept -> logged & = delete;
			inline ~logged(void) noexcept = default;
		public:
			system_component::file _file;
			system_component::lock::critical_section _lock;
		};
	private:
		inline explicit logger(void) noexcept = default;
		inline explicit logger(logger const&) noexcept = delete;
		inline explicit logger(logger&&) noexcept = delete;
		inline auto operator=(logger const&) noexcept -> logger & = delete;
		inline auto operator=(logger&&) noexcept -> logger & = delete;
		inline ~logger(void) noexcept = default;
	public:
		inline void create(std::string_view key, std::wstring path) noexcept {
			auto& iter = _logged.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple()).first->second;
			unsigned long attribute = system_component::file::get_attribute(path.data());
			if (attribute == INVALID_FILE_ATTRIBUTES || (attribute & FILE_ATTRIBUTE_DIRECTORY)) {
				for (size_type index = 0; index < path.size(); ++index) {
					if (L'\\' == path[index]) {
						path[index] = L'\0';
						system_component::file::create_directory(path.data());
						path[index] = L'\\';
					}
				}
				iter._file.create(path.data(), FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH);
				wchar_t const bom = 0xfeff;
				iter._file.write(&bom, sizeof(bom));
			}
			else {
				iter._file.create(path.data(), FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH);
				iter._file.set_pointer_ex(LARGE_INTEGER{ 0 }, FILE_END);
			}
		}
		template <level level_>
		inline void log_message(std::string_view const key, wchar_t const* const format, ...) noexcept {
			//if (0 != _output && level_ >= _level) {
			system_component::time::unix unix(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
			system_component::time::date date(unix.local_time());

			wchar_t buffer[256];
			int length = _snwprintf_s(buffer, 255, 255, L"[%d-%02d-%02d %02d:%02d:%02d / %s / %08d] ", date.year(), date.month(), date.day_of_month(), date.hour(), date.minute(), date.second(), lm[static_cast<unsigned char>(level_)], _InterlockedIncrement(&_count));
			va_list va_list_;
			va_start(va_list_, format);
			length += _vsnwprintf_s(buffer + length, 255 - length, 255 - length, format, va_list_);
			va_end(va_list_);
			length += _snwprintf_s(buffer + length, 255 - length, 255 - length, L"\n");

			if (static_cast<unsigned char>(output::console) & _output)
				wprintf(buffer);
			if (static_cast<unsigned char>(output::file) & _output) {
				auto& iter = _logged.find(key.data())->second;
				iter._lock.enter();
				iter._file.write(buffer, static_cast<unsigned long>(length) * 2);
				iter._lock.leave();
			}
			//}
		}
		template<level level_>
		inline void log_memory(std::string_view const key, wchar_t const* const print, unsigned char* pointer, size_type size, size_type const column) noexcept {
			if (0 != _output && level_ >= _level) {
				system_component::time::unix unix(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
				system_component::time::date date(unix.local_time());

				wchar_t buffer[256];
				int length = _snwprintf_s(buffer, 255, 255, L"[%d-%02d-%02d %02d:%02d:%02d / %s / %08d] ", date.year(), date.month(), date.day_of_month(), date.hour(), date.minute(), date.second(), lm[static_cast<unsigned char>(level_)], _InterlockedIncrement(&_count));
				length += _snwprintf_s(buffer + length, 255 - length, 255 - length, L"%s", print);
				for (size_type index = 0; index < size; index++) {
					if (0 == index % column)
						length += _snwprintf_s(buffer + length, 255 - length, 255 - length, L"\n%p  ", pointer + index);
					length += _snwprintf_s(buffer + length, 255 - length, 255 - length, L"%02x ", pointer[index]);
				}
				size += _snwprintf_s(buffer + length, 255 - length, 255 - length, L"\n");

				if (static_cast<unsigned char>(output::console) & _output)
					wprintf(buffer);
				if (static_cast<unsigned char>(output::file) & _output) {
					auto& iter = _logged.find(key.data())->second;
					iter._lock.enter();
					iter._file.write(buffer, static_cast<unsigned long>(length) * 2);
					iter._lock.leave();
				}
			}
		}
		inline void set_output(unsigned char const output_) noexcept {
			_output = output_;
		}
		inline auto get_output(void) const noexcept -> unsigned char {
			return _output;
		}
		inline void set_level(level const level_) noexcept {
			_level = level_;
		}
		inline auto get_level(void) const noexcept -> level {
			return _level;
		}
	private:
		unsigned char _output = 0;
		level _level = static_cast<level>(none);
		volatile size_type _count = 0;
		std::unordered_map<std::string, logged> _logged;
	};
}

#define log_message(key, level, format, ...)\
{\
	auto& logger = utility::logger::instance();\
	if(0 != logger.get_output() && level >= logger.get_level()) {\
		logger.log_message<level>(key, format, ##__VA_ARGS__);\
	}\
}