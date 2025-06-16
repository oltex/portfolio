#pragma once
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

namespace system_component {
	class socket_address {
	public:
		inline explicit socket_address(void) noexcept = default;
		inline explicit socket_address(socket_address const& rhs) noexcept = default;
		inline explicit socket_address(socket_address&& rhs) noexcept = default;
		inline auto operator=(socket_address const& rhs) noexcept -> socket_address & = default;
		inline auto operator=(socket_address&& rhs) noexcept -> socket_address & = default;
		inline ~socket_address(void) noexcept = default;

		inline virtual ADDRESS_FAMILY get_family(void) const noexcept = 0;
		inline virtual int get_length(void) const noexcept = 0;
		inline virtual sockaddr& data(void) noexcept = 0;
	};

	class socket_address_ipv4 final : public socket_address {
	public:
		inline explicit socket_address_ipv4(void) noexcept
			: _sockaddr{} {
			_sockaddr.sin_family = AF_INET;
		}
		inline socket_address_ipv4(socket_address_ipv4 const& rhs) noexcept
			: _sockaddr(rhs._sockaddr) {
		};
		inline explicit socket_address_ipv4(socket_address_ipv4&& rhs) noexcept
			: _sockaddr(rhs._sockaddr) {
		};
		inline auto operator=(socket_address_ipv4 const& rhs) noexcept -> socket_address_ipv4& {
			_sockaddr = rhs._sockaddr;
			return *this;
		};
		inline auto operator=(socket_address_ipv4&& rhs) noexcept -> socket_address_ipv4& {
			_sockaddr = rhs._sockaddr;
			return *this;
		};
		inline ~socket_address_ipv4(void) noexcept = default;

		inline void set_address(unsigned long address) noexcept {
			_sockaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		}
		inline void set_address(char const* const address) noexcept {
			if (1 != inet_pton(AF_INET, address, &_sockaddr.sin_addr))
				__debugbreak();
		}
		inline auto get_address(void) const noexcept -> std::wstring {
			wchar_t string[INET_ADDRSTRLEN];
			if (0 == InetNtopW(AF_INET, &_sockaddr.sin_addr, string, INET_ADDRSTRLEN))
				__debugbreak();
			return std::wstring(string);
		}
		inline void set_port(unsigned short port) noexcept {
			_sockaddr.sin_port = htons(port);
		}
		inline auto get_port(void) const noexcept -> unsigned short {
			return ntohs(_sockaddr.sin_port);
		}

		inline virtual ADDRESS_FAMILY get_family(void) const noexcept override {
			return AF_INET;
		}
		inline virtual int get_length(void) const noexcept override {
			return sizeof(sockaddr_in);
		}
		inline virtual sockaddr& data(void) noexcept override {
			return *reinterpret_cast<sockaddr*>(&_sockaddr);
		}
	private:
		sockaddr_in _sockaddr;
	};

	class socket_address_storage final : public socket_address {
	public:
		inline explicit socket_address_storage(void) noexcept
			:_sockaddr{} {
		};
		inline explicit socket_address_storage(socket_address_storage const& rhs) noexcept
			: _sockaddr(rhs._sockaddr) {
		};
		inline explicit socket_address_storage(socket_address_storage&& rhs) noexcept
			: _sockaddr(rhs._sockaddr) {
		};
		inline auto operator=(socket_address_storage const& rhs) noexcept -> socket_address_storage& {
			_sockaddr = rhs._sockaddr;
			return *this;
		};
		inline auto operator=(socket_address_storage&& rhs) noexcept -> socket_address_storage& {
			_sockaddr = rhs._sockaddr;
			return *this;
		};
		inline ~socket_address_storage(void) noexcept = default;

		inline virtual ADDRESS_FAMILY get_family(void) const noexcept {
			return _sockaddr.ss_family;
		}
		inline virtual int get_length(void) const noexcept override {
			switch (get_family()) {
			case AF_INET:
				return sizeof(sockaddr_in);
			}
			return sizeof(sockaddr_storage);
		}
		inline virtual sockaddr& data(void) noexcept override {
			return *reinterpret_cast<sockaddr*>(&_sockaddr);
		}
	protected:
		sockaddr_storage _sockaddr;
	};
}