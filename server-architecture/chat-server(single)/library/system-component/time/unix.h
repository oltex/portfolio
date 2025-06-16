#pragma once
#include <ctime>

namespace system_component::time {
	class unix final {
	public:
		inline explicit unix(void) noexcept = default;
		inline explicit unix(__time64_t time_t) noexcept
			: _time_t(time_t) {
		};
		inline explicit unix(unix const& rhs) noexcept
			: _time_t(rhs._time_t) {
		}
		inline explicit unix(unix&& rhs) noexcept
			: _time_t(rhs._time_t) {
		}
		inline auto operator=(unix const& rhs) noexcept -> unix& {
			_time_t = rhs._time_t;
		}
		inline auto operator=(unix&& rhs) noexcept -> unix& {
			_time_t = rhs._time_t;
		}
		inline ~unix(void) noexcept = default;
	public:
		inline void time(void) noexcept {
			_time64(&_time_t);
		}
		inline auto local_time(void) const noexcept -> tm {
			std::tm tm;
			_localtime64_s(&tm, &_time_t);
			return tm;
		}
	public:
		inline auto data(void) noexcept -> __time64_t& {
			return _time_t;
		}
	private:
		__time64_t _time_t;
	};
}