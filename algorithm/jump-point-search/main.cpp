#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include "application.h"

application* _application;

auto message(void) noexcept -> MSG;
LRESULT CALLBACK procedure(HWND const wnd, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
int APIENTRY wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	WNDCLASSEXW _wcex;
	memset(&_wcex, 0, sizeof(WNDCLASSEX));
	_wcex.cbSize = sizeof(WNDCLASSEX);
	_wcex.lpfnWndProc = DefWindowProcW;
	_wcex.hbrBackground = 0;
	_wcex.hInstance = hinstance;
	_wcex.lpszClassName = L"window";
	_wcex.lpfnWndProc = procedure;
	_wcex.style = CS_HREDRAW | CS_VREDRAW;
	_wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
	RegisterClassExW(&_wcex);

	HWND hwnd = CreateWindowExW(
		0, L"window", nullptr, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr,
		nullptr, hinstance, nullptr);

	_application = new application(hwnd);
	_application->initialize();

	MSG msg = message();

	delete _application;
	return static_cast<int>(msg.wParam);
}

auto message(void) noexcept -> MSG {
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return msg;
};

LRESULT CALLBACK procedure(HWND const hwnd, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept {
	switch (message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_KEYDOWN:
	case WM_TIMER:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_PAINT:
		_application->procedure(hwnd, message, wparam, lparam);
		break;
	case WM_DESTROY: {
		PostQuitMessage(0);
	} break;
	default:
		return DefWindowProc(hwnd, message, wparam, lparam);
	}
	return 0;
}