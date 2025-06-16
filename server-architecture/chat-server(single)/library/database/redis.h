#pragma once
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

namespace database {
	class redis final : public cpp_redis::client {
	public:
		inline explicit redis(void) noexcept = default;
		inline explicit redis(redis const&) noexcept = delete;
		inline explicit redis(redis&&) noexcept = delete;
		inline auto operator=(redis const&) noexcept -> redis & = delete;
		inline auto operator=(redis&&) noexcept -> redis & = delete;
		inline ~redis(void) noexcept {
			disconnect();
		};
	};
}