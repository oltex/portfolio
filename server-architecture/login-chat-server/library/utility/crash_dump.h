#pragma once
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

#include <iostream>
#include <intrin.h>
#include <new.h>
#include <eh.h>

#include <chrono>
#include <string>

namespace utility {
	inline static auto _stdcall unhandled_exception_filter(__in EXCEPTION_POINTERS* exception_pointer) noexcept -> long {
		volatile static unsigned int _lock = 0;
		if (0 == _InterlockedExchange(&_lock, 1)) {
			__time64_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm; _localtime64_s(&tm, &time);
			std::wostringstream stream;
			stream << std::put_time(&tm, L"%y%m%d_%H%M%S") << L".dmp";

			HANDLE file = CreateFileW(stream.str().data(), FILE_WRITE_DATA, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);

			_MINIDUMP_EXCEPTION_INFORMATION minidump_exception_information;
			minidump_exception_information.ThreadId = GetCurrentThreadId();
			minidump_exception_information.ExceptionPointers = exception_pointer;
			minidump_exception_information.ClientPointers = true;
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithFullMemory, &minidump_exception_information, nullptr, nullptr);

			CloseHandle(file);
		}
		return 0;
	};
	inline static void crash_dump(void) noexcept {
		_set_invalid_parameter_handler([](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) noexcept -> void { __debugbreak(); });
		//_set_thread_local_invalid_parameter_handler([](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) -> void { __debugbreak(); });
		_set_purecall_handler([](void) noexcept -> void { __debugbreak(); });

		//std::set_terminate() //설명: 프로그램에서 예외가 잡히지 않았을 때 호출되는 종료 처리기를 설정합니다. 사용: terminate() 함수 호출 시 실행될 핸들러를 등록.결론:작동안함
		//std::set_new_handler() //설명: operator new가 메모리 할당에 실패했을 때 호출될 핸들러를 설정합니다. 사용: 메모리 부족 시 특정 처리를 실행.
		//_set_abort_behavior() //설명: abort 호출 시 동작을 제어합니다. 사용 : 런타임에서 디버그 메시지 출력 여부를 설정. 결론:에러로 안잡아줌 어떻게 설정해도
		//_set_se_translator() //설명: 구조화된 예외를 C++ 예외로 변환하는 핸들러를 설정합니다. 사용 : Windows 환경에서 SEH를 표준 C++ 예외로 처리. 결론:에러가 아니다 예외처리 방식이라 이때 덤프를 낼 이유는 없음

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportHook([](int, char*, int*) noexcept -> int { __debugbreak(); return 0; });
		//_CrtSetReportHook2();
		//_CrtSetReportHookW2();

		SetUnhandledExceptionFilter(unhandled_exception_filter);
	};
}