#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <iostream>
#include <thread>
#include <vector>


#include "tls_pool.h"

LARGE_INTEGER _frequency;
long long _sum = 0;
long long _count = 0;

struct my_str {
	inline explicit my_str(void) noexcept = delete;
	inline ~my_str(void) noexcept = delete;
	unsigned char _buffer[256];
};

inline static unsigned int __stdcall func(void* arg) noexcept {
	auto& _pool = library::_thread_local::pool<my_str>::instance();
	my_str** _array = reinterpret_cast<my_str**>(malloc(sizeof(my_str*) * 10000));
	if (nullptr == _array)
		__debugbreak();
	LARGE_INTEGER _start;
	LARGE_INTEGER _end;
	for (;;) {
		QueryPerformanceCounter(&_start);
		for (int i = 0; i < 10000; ++i) {
			for (auto j = 0; j < 5000; ++j)
				_array[j] = &_pool.allocate();
			for (auto j = 0; j < 5000; ++j)
				_pool.deallocate(*_array[j]);
		}
		QueryPerformanceCounter(&_end);
		printf("%f\n", (_end.QuadPart - _start.QuadPart) / static_cast<double>(_frequency.QuadPart) * 1e3);
	}
	free(_array);
	return 0;
}
inline static unsigned int __stdcall func2(void* arg) noexcept {
	auto& _pool = library::_thread_local::pool<int>::instance();
	my_str** _array = reinterpret_cast<my_str**>(malloc(sizeof(my_str*) * 10000));
	if (nullptr == _array)
		__debugbreak();
	LARGE_INTEGER _start;
	LARGE_INTEGER _end;
	for (;;) {
		QueryPerformanceCounter(&_start);
		for (int i = 0; i < 10000; ++i) {
			for (auto j = 0; j < 5000; ++j)
				_array[j] = reinterpret_cast<my_str*>(malloc(sizeof(my_str)));
			for (auto j = 0; j < 5000; ++j)
				free(_array[j]);
		}
		QueryPerformanceCounter(&_end);
		printf("%f\n", (_end.QuadPart - _start.QuadPart) / static_cast<double>(_frequency.QuadPart) * 1e3);
	}
	free(_array);
	return 0;
}

int main(void) noexcept {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	QueryPerformanceFrequency(&_frequency);

	int count;
	scanf_s("%d", &count);
	for (int i = 0; i < count; ++i) {
		(HANDLE)_beginthreadex(nullptr, 0, func, nullptr, 0, 0);
	}
	system("pause");
	return 0;
}