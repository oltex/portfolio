#pragma once
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include "socket.h"
#include "handle.h"

namespace system_component {
	class inputoutput_completion_port final : public handle {
	public:
		inline explicit inputoutput_completion_port(void) noexcept = default;
		inline explicit inputoutput_completion_port(unsigned long const concurrent_thread) noexcept
			: handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_thread)) {
		};
		inline explicit inputoutput_completion_port(inputoutput_completion_port const&) noexcept = delete;
		inline explicit inputoutput_completion_port(inputoutput_completion_port&& rhs) noexcept
			: handle(std::move(rhs)) {
		};
		inline auto operator=(inputoutput_completion_port const&) noexcept -> inputoutput_completion_port & = delete;
		inline auto operator=(inputoutput_completion_port&& rhs) noexcept -> inputoutput_completion_port& {
			handle::operator=(std::move(rhs));
			return *this;
		}
		inline virtual ~inputoutput_completion_port(void) noexcept override = default;
	public:
		inline void create(unsigned long const concurrent_thread) noexcept {
			_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_thread);
		}
		inline void connect(socket& socket, ULONG_PTR key) noexcept {
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket.data()), _handle, key, 0);
		}
		struct get_queue_state_result final {
			bool _result;
			DWORD _transferred;
			ULONG_PTR _key;
			OVERLAPPED* _overlapped;
		};
		inline auto get_queue_state(unsigned long milli_second) noexcept -> get_queue_state_result {
			get_queue_state_result result;
			result._result = GetQueuedCompletionStatus(_handle, &result._transferred, &result._key, &result._overlapped, milli_second);
			return result;
		}
		inline void post_queue_state(unsigned long transferred, uintptr_t key, OVERLAPPED* overlapped) noexcept {
			PostQueuedCompletionStatus(_handle, transferred, key, overlapped);
		}
	};
}