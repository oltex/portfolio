#pragma once
#include <Windows.h>
#pragma comment(lib, "Winmm.lib")

namespace system_component::time::multimedia {
	inline static auto get_time(void) noexcept -> unsigned long {
		return timeGetTime();
	}
	inline static void begin_period(unsigned int peroid) noexcept {
		timeBeginPeriod(peroid);
	}
	inline static void end_period(unsigned int peroid) noexcept {
		timeEndPeriod(peroid);
	}
};
