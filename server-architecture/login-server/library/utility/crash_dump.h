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

		//std::set_terminate() //����: ���α׷����� ���ܰ� ������ �ʾ��� �� ȣ��Ǵ� ���� ó���⸦ �����մϴ�. ���: terminate() �Լ� ȣ�� �� ����� �ڵ鷯�� ���.���:�۵�����
		//std::set_new_handler() //����: operator new�� �޸� �Ҵ翡 �������� �� ȣ��� �ڵ鷯�� �����մϴ�. ���: �޸� ���� �� Ư�� ó���� ����.
		//_set_abort_behavior() //����: abort ȣ�� �� ������ �����մϴ�. ��� : ��Ÿ�ӿ��� ����� �޽��� ��� ���θ� ����. ���:������ ������� ��� �����ص�
		//_set_se_translator() //����: ����ȭ�� ���ܸ� C++ ���ܷ� ��ȯ�ϴ� �ڵ鷯�� �����մϴ�. ��� : Windows ȯ�濡�� SEH�� ǥ�� C++ ���ܷ� ó��. ���:������ �ƴϴ� ����ó�� ����̶� �̶� ������ �� ������ ����

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportHook([](int, char*, int*) noexcept -> int { __debugbreak(); return 0; });
		//_CrtSetReportHook2();
		//_CrtSetReportHookW2();

		SetUnhandledExceptionFilter(unhandled_exception_filter);
	};
}