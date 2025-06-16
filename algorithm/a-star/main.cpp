#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "../../system/window/instance.h"
#include "../../system/window/cls.h"
#include "../../system/window/sct.h"
#include "../../system/window/window.h"

#include "client.h"
//ºê·¹ÁðÇÜ ÇÏ°í

auto message(void) noexcept -> MSG;
LRESULT CALLBACK procedure(HWND const wnd, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
int APIENTRY wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	window::instance instance(hinstance);
	window::cursor cursor;
	cursor.load(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

	window::cls cls;
	cls.set_instance(instance);
	cls.set_class_name(L"window");
	cls.set_procedure(procedure);
	cls.set_style(CS_HREDRAW | CS_VREDRAW);
	cls.set_cursor(cursor);
	cls.register_();

	window::sct sct;
	sct.set_instance(instance);
	sct.set_class_name(L"window");
	sct.set_style(WS_OVERLAPPEDWINDOW);
	sct.set_x(CW_USEDEFAULT);
	sct.set_width(CW_USEDEFAULT);

	client::constructor(sct.create());
	client::instance().initialize();

	MSG msg = message();
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
		client::instance().procedure(hwnd, message, wparam, lparam);
		break;
	case WM_DESTROY: {
		PostQuitMessage(0);
	} break;
	default:
		return DefWindowProc(hwnd, message, wparam, lparam);
	}
	return 0;
}