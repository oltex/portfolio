#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "lockfree_queue.h"
#include <thread>
#include <intrin.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <Windows.h>

library::lockfree::queue<int> _lockfree_queue;
std::queue<int> std_queue;

alignas(64) SRWLOCK _srw;
LARGE_INTEGER _frequency;

inline static unsigned int __stdcall func1(void* arg) noexcept {
	LARGE_INTEGER _start;
	LARGE_INTEGER _end;
	long long _sum = 0;
	long long _count = 0;
	for (;;) {
		for (int i = 0; i < 100000; ++i) {
			for (auto j = 0; j < 500; ++j) {
				QueryPerformanceCounter(&_start);
				_lockfree_queue.emplace(0);
				QueryPerformanceCounter(&_end);
				_sum += _end.QuadPart - _start.QuadPart;
				for (volatile int k = 0; k < 8; ++k) {
				}
			}
			for (auto j = 0; j < 500; ++j) {
				QueryPerformanceCounter(&_start);
				_lockfree_queue.pop();
				QueryPerformanceCounter(&_end);
				_sum += _end.QuadPart - _start.QuadPart;
				for (volatile int k = 0; k < 8; ++k) {
				}
			}
		}
		++_count;
		printf("%f\n", (static_cast<double>(_sum) / _count) / static_cast<double>(_frequency.QuadPart) * 1e3);
	}
	return 0;
}
inline static unsigned int __stdcall func2(void* arg) noexcept {
	LARGE_INTEGER _start;
	LARGE_INTEGER _end;
	long long _sum = 0;
	long long _count = 0;
	for (;;) {
		for (int i = 0; i < 100000; ++i) {
			for (auto j = 0; j < 500; ++j) {
				QueryPerformanceCounter(&_start);
				AcquireSRWLockExclusive(&_srw);
				std_queue.push(0);
				ReleaseSRWLockExclusive(&_srw);
				QueryPerformanceCounter(&_end);
				_sum += _end.QuadPart - _start.QuadPart;
				for (volatile int k = 0; k < 8; ++k) {
				}
			}
			for (auto j = 0; j < 500; ++j) {
				QueryPerformanceCounter(&_start);
				AcquireSRWLockExclusive(&_srw);
				std_queue.pop();
				ReleaseSRWLockExclusive(&_srw);
				QueryPerformanceCounter(&_end);
				_sum += _end.QuadPart - _start.QuadPart;
				for (volatile int k = 0; k < 8; ++k) {
				}
			}
		}
		++_count;
		printf("%f\n", (static_cast<double>(_sum) / _count) / static_cast<double>(_frequency.QuadPart) * 1e3);
	}
	return 0;
}

int main(void) noexcept {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	QueryPerformanceFrequency(&_frequency);
	InitializeSRWLock(&_srw);

	int count;
	scanf_s("%d", &count);
	for (int i = 0; i < count; ++i) {
		(HANDLE)_beginthreadex(nullptr, 0, func2, nullptr, 0, 0);
	}
	Sleep(INFINITE);
	return 0;
}