#pragma once
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <intrin.h>
#include <optional>
#include "socket_address.h"
#include "overlapped.h"
#include "../data-structure/pair.h"

namespace system_component {
	inline static void wsa_start_up(void) noexcept {
		WSAData wsadata;
		if (0 != WSAStartup(0x0202, &wsadata))
			__debugbreak();
	};
	inline static void wsa_clean_up(void) noexcept {
		if (SOCKET_ERROR == WSACleanup())
			__debugbreak();
	};

	class socket final {
	public:
		inline explicit socket(void) noexcept
			: _socket(INVALID_SOCKET) {
		}
		inline explicit socket(ADDRESS_FAMILY const address_family, int const type, int const protocol) noexcept
			: _socket(::socket(address_family, type, protocol)) {
			if (INVALID_SOCKET == _socket) {
				switch (GetLastError()) {
				case WSANOTINITIALISED:
				default:
					__debugbreak();
				}
			}
		}
		inline explicit socket(SOCKET const sock) noexcept
			: _socket(sock) {
		}
		inline explicit socket(socket const&) noexcept = delete;
		inline explicit socket(socket&& rhs) noexcept
			: _socket(rhs._socket) {
			rhs._socket = INVALID_SOCKET;
		}
		inline auto operator=(socket const&) noexcept -> socket & = delete;
		inline auto operator=(socket&& rhs) noexcept -> socket& {
			closesocket(_socket);
			_socket = rhs._socket;
			rhs._socket = INVALID_SOCKET;
			return *this;
		};
		inline ~socket(void) noexcept {
			closesocket(_socket);
		}

		inline void create(ADDRESS_FAMILY const address_family, int const type, int const protocol) noexcept {
			_socket = ::socket(address_family, type, protocol);
			if (INVALID_SOCKET == _socket) {
				switch (GetLastError()) {
				case WSANOTINITIALISED:
				default:
					__debugbreak();
				}
			}
		}
		inline auto bind(socket_address& socket_address) const noexcept -> int {
			int result = ::bind(_socket, &socket_address.data(), socket_address.get_length());
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAEADDRINUSE:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline auto listen(int backlog) const noexcept -> int {
			if (SOMAXCONN != backlog)
				backlog = SOMAXCONN_HINT(backlog);
			int result = ::listen(_socket, backlog);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAEINVAL:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline auto accept(void) const noexcept -> data_structure::pair<socket, socket_address_ipv4> {
			socket_address_ipv4 socket_address;
			int length = sizeof(sockaddr_in);
			SOCKET sock = ::accept(_socket, &socket_address.data(), &length);
			if (INVALID_SOCKET == sock) {
				switch (GetLastError()) {
				case WSAEWOULDBLOCK:
				case WSAEINVAL:
				case WSAENOTSOCK:
				case WSAEINTR:
					break;
				default:
					__debugbreak();
				}
			}
			return data_structure::pair<socket, socket_address_ipv4>(socket(sock), socket_address);
		}
		inline auto connect(socket_address& socket_address) noexcept -> int {
			int result = ::connect(_socket, &socket_address.data(), socket_address.get_length());
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAEWOULDBLOCK:
					break;
				case WSAETIMEDOUT:
				case WSAECONNREFUSED:
					close();
					break;
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline void shutdown(int const how) const noexcept {
			if (SOCKET_ERROR == ::shutdown(_socket, how))
				__debugbreak();
		}
		inline void close(void) noexcept {
			closesocket(_socket);
			_socket = INVALID_SOCKET;
		}
		inline auto send(char const* const buffer, int const length, int const flag) noexcept -> int {
			int result = ::send(_socket, buffer, length, flag);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAEWOULDBLOCK:
					break;
				case WSAECONNRESET:
				case WSAECONNABORTED:
					close();
					break;
				case WSAENOTCONN:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline auto send_to(char const* const buffer, int const length, int const flag, socket_address& socket_address) const noexcept -> int {
			return ::sendto(_socket, buffer, length, flag, &socket_address.data(), socket_address.get_length());
		}
		inline auto wsa_send(WSABUF* buffer, unsigned long count, unsigned long* byte, unsigned long flag) noexcept -> int {
			int result = WSASend(_socket, buffer, count, byte, flag, nullptr, nullptr);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAECONNRESET:
				case WSAECONNABORTED:
					close();
					break;
				case WSAENOTSOCK:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline auto wsa_send(WSABUF* buffer, unsigned long count, unsigned long flag, overlapped& overlapped) noexcept -> int {
			int result = WSASend(_socket, buffer, count, nullptr, flag, &overlapped.data(), nullptr);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSA_IO_PENDING:
				case WSAECONNRESET:
				case WSAECONNABORTED:
				case WSAEINTR:
				case WSAEINVAL:
					break;
				case WSAENOTSOCK:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline auto receive(char* const buffer, int const length, int const flag) noexcept -> int {
			int result = ::recv(_socket, buffer, length, flag);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAEWOULDBLOCK:
					break;
				case WSAECONNRESET:
				case WSAECONNABORTED:
					close();
					break;
				case WSAENOTCONN:
				default:
					__debugbreak();
				}
			}
			else if (0 == result)
				close();
			return result;
		}
		inline auto receive_from(char* const buffer, int const length, int const flag, socket_address& socket_address, int& from_length) noexcept -> int {
			return ::recvfrom(_socket, buffer, length, flag, &socket_address.data(), &from_length);
		}
		inline auto wsa_receive(WSABUF* buffer, unsigned long count, unsigned long* byte, unsigned long* flag) noexcept -> int {
			int result = WSARecv(_socket, buffer, count, byte, flag, nullptr, nullptr);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSAECONNRESET:
				case WSAECONNABORTED:
					close();
					break;
				case WSAENOTSOCK:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline auto wsa_receive(WSABUF* buffer, unsigned long count, unsigned long* flag, overlapped& overlapped) noexcept -> int {
			int result = WSARecv(_socket, buffer, count, nullptr, flag, &overlapped.data(), nullptr);
			if (SOCKET_ERROR == result) {
				switch (GetLastError()) {
				case WSA_IO_PENDING:
				case WSAECONNRESET:
				case WSAECONNABORTED:
					break;
				case WSAENOTSOCK:
				default:
					__debugbreak();
				}
			}
			return result;
		}
		inline void cancel_io(void) const noexcept {
			CancelIo(reinterpret_cast<HANDLE>(_socket));
		}
		inline void cancel_io_ex(void) const noexcept {
			CancelIoEx(reinterpret_cast<HANDLE>(_socket), nullptr);
		}
		inline void cancel_io_ex(overlapped overlapped) const noexcept {
			CancelIoEx(reinterpret_cast<HANDLE>(_socket), &overlapped.data());
		}
		inline bool wsa_get_overlapped_result(overlapped& overlapped, unsigned long* transfer, bool const wait, unsigned long* flag) noexcept {
			return WSAGetOverlappedResult(_socket, &overlapped.data(), transfer, wait, flag);
		}
		inline void set_tcp_nodelay(int const enable) const noexcept {
			set_option(IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char const*>(&enable), sizeof(int));
		}
		inline void set_linger(unsigned short const onoff, unsigned short const time) const noexcept {
			LINGER linger{ onoff , time };
			set_option(SOL_SOCKET, SO_LINGER, reinterpret_cast<char const*>(&linger), sizeof(LINGER));
		}
		inline void set_broadcast(unsigned long const enable) const noexcept {
			set_option(SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char const*>(&enable), sizeof(unsigned long));
		}
		inline void set_send_buffer(int const size) const noexcept {
			set_option(SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char const*>(&size), sizeof(size));
		}
		inline void set_receive_buffer(int const size) const noexcept {
			set_option(SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const*>(&size), sizeof(size));
		}
		inline void set_option(int const level, int const name, char const* value, int const length) const noexcept {
			if (SOCKET_ERROR == setsockopt(_socket, level, name, value, length))
				__debugbreak();
		}
		inline void set_nonblocking(unsigned long const enable) const noexcept {
			set_io_control(FIONBIO, enable);
		}
		inline void set_io_control(long const cmd, unsigned long arg) const noexcept {
			if (SOCKET_ERROR == ioctlsocket(_socket, cmd, &arg))
				__debugbreak();
		}
		inline auto get_local_socket_address(void) const noexcept -> std::optional<socket_address_ipv4> {
			socket_address_ipv4 socket_address;
			int length = socket_address.get_length();
			if (SOCKET_ERROR == getsockname(_socket, &socket_address.data(), &length)) {
				switch (GetLastError()) {
				default:
					break;
#pragma warning(suppress: 4065)
				}
				return std::nullopt;
			}
			return socket_address;
		}
		inline auto get_remote_socket_address(void) const noexcept -> std::optional<socket_address_ipv4> {
			socket_address_ipv4 socket_address;
			int length = socket_address.get_length();
			if (SOCKET_ERROR == getpeername(_socket, &socket_address.data(), &length)) {
				switch (GetLastError()) {
				default:
					break;
#pragma warning(suppress: 4065)
				}
				return std::nullopt;
			}
			return socket_address;
		}
		inline auto data(void) noexcept -> SOCKET& {
			return _socket;
		}
	private:
		SOCKET _socket;
	};
}