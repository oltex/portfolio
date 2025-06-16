#pragma once
#include "server.h"

class echo_server final : public server {
public:
	class my_group : public server::group {
	public:
		inline my_group(void) noexcept {
			int a = 10;
		};
		inline ~my_group(void) noexcept {
			int a = 10;
		};
		inline virtual void on_monit(void) noexcept override {
		}
		inline virtual void on_enter_session(unsigned long long key) noexcept override {
		};
		inline virtual bool on_receive_session(unsigned long long key, view_pointer& view_ptr) noexcept override {
			return true;
		};
		inline virtual void on_leave_session(unsigned long long key) noexcept override {
		};
		inline virtual int on_update(void) noexcept override {
			return 20;
		};
	};
public:
	inline explicit echo_server(void) noexcept {
	};
	inline explicit echo_server(echo_server const&) noexcept = delete;
	inline explicit echo_server(echo_server&&) noexcept = delete;
	inline auto operator=(echo_server const&) noexcept -> echo_server & = delete;
	inline auto operator=(echo_server&&) noexcept -> echo_server & = delete;
	inline ~echo_server(void) noexcept = default;

	inline virtual void on_start(void) noexcept override {
		_group_key = do_create_group<my_group>();
	}
	inline virtual void on_worker_start(void) noexcept override {
	}
	inline virtual void on_stop(void) noexcept override {
		do_destroy_group(_group_key);
	}
	inline virtual void on_monit(void) noexcept override {
	}

	inline virtual bool on_accept_socket(system_component::socket_address_ipv4& socket_address) noexcept override {
		return true;
	}
	inline virtual void on_create_session(unsigned long long key) noexcept override {
		//message_pointer message_ = server::create_message();
		//*message_ << 0x7fffffffffffffff;
		//do_send_session(key, message_);
	}
	inline virtual bool on_receive_session(unsigned long long key, view_pointer& view_ptr) noexcept override {
		unsigned long long value;
		*view_ptr >> value;
		view_pointer message_ = create_message(8);
		*message_ << value;
		do_set_timeout_session(key, 40000);
		do_send_session(key, message_);
		return true;
	}
	inline virtual void on_destroy_session(unsigned long long key) noexcept override {
	}
	inline virtual void on_destory_group(unsigned long long key) noexcept override {
	};

	unsigned long long _group_key;
};