#pragma once
#include <iostream>
#include <iomanip>
#include <ctime>

namespace system_component::time {
	class date final {
	public:
		inline explicit date(void) noexcept = default;
		inline explicit date(std::tm tm) noexcept
			: _tm(tm) {
		};
		inline explicit date(date const& rhs) noexcept
			: _tm(rhs._tm) {
		};
		inline explicit date(date&& rhs) noexcept
			: _tm(rhs._tm) {
		}
		inline auto operator=(date const& rhs) noexcept -> date& {
			_tm = rhs._tm;
		}
		inline auto operator=(date&& rhs) noexcept -> date& {
			_tm = rhs._tm;
		}
		inline ~date(void) noexcept = default;
	public:
		inline auto put_time(char const* const format) const noexcept {
			return std::put_time(&_tm, format);
		}
		inline auto year(void) const noexcept -> int {
			return _tm.tm_year - 100;
		}
		inline auto month(void) const noexcept -> int {
			return _tm.tm_mon + 1;
		}
		inline auto day_of_year(void) const noexcept -> int {
			return _tm.tm_yday + 1;
		}
		inline auto day_of_month(void) const noexcept -> int {
			return _tm.tm_mday;
		}
		inline auto day_of_week(void) const noexcept -> int {
			return _tm.tm_wday;
		}
		inline auto hour(void) const noexcept -> int {
			return _tm.tm_hour;
		}
		inline auto minute(void) const noexcept -> int {
			return _tm.tm_min;
		}
		inline auto second(void) const noexcept -> int {
			return _tm.tm_sec;
		}
	private:
		std::tm _tm;
	};
}