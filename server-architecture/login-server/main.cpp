#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include "login_server.h"

int main(void) noexcept {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	auto& login_server_ = login_server::instance();
	{
		command::parameter param("include", "server.cfg");
		command::instance().execute("include", &param);
	}
	system("pause");
	{
		command::parameter param("server_stop");
		command::instance().execute("server_stop", &param);
	}
}