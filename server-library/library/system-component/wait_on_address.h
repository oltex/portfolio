#pragma once
#pragma comment(lib, "Synchronization.lib")
#include <Windows.h>

namespace system_component::wait_on_address {
	inline static bool wait(void* address, void* _compare, size_t const size, unsigned long const milli_second) noexcept {
		return WaitOnAddress(address, _compare, size, milli_second);
	}
	inline static void wake_single(void* address) noexcept {
		WakeByAddressSingle(address);
	}
	inline static void wake_all(void* address) noexcept {
		WakeByAddressAll(address);
	}
}