#pragma once
#include "server.h"
#include "library/design-pattern/singleton.h"
#include "CommonProtocol.h"

class login_server final : public server, public design_pattern::singleton<login_server> {
private:
	friend class design_pattern::singleton<login_server>;
private:
	inline explicit login_server(void) noexcept = default;
	inline explicit login_server(login_server const&) noexcept = delete;
	inline explicit login_server(login_server&&) noexcept = delete;
	inline auto operator=(login_server const&) noexcept -> login_server & = delete;
	inline auto operator=(login_server&&) noexcept -> login_server & = delete;
	inline ~login_server(void) noexcept = default;
public:
	virtual void on_start(void) noexcept override {
		_monitor_server = do_create_session("127.0.0.1", 10405);
		auto msg_ptr = create_message(6);
		*msg_ptr << (unsigned short)en_PACKET_SS_MONITOR_LOGIN << (int)0;
		do_send_session(_monitor_server, msg_ptr);
	};
	virtual void on_stop(void) noexcept override {
	};
	virtual void on_worker_start(void) noexcept {
		auto& mysql = design_pattern::_thread_local::singleton<database::mysql>::instance();
		auto& redis = design_pattern::_thread_local::singleton<database::redis>::instance();
		mysql.connect("10.0.1.2", "root", "1111", "accountdb", 3306);
		redis.connect("10.0.2.2", 6379, nullptr, 0, 0, 0);
		//mysql.connect("127.0.0.1", "root", "1111", "accountdb", 3306);
		//redis.connect("127.0.0.1", 6379, nullptr, 0, 0, 0);
	}
	virtual void on_monit(void) noexcept override {
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, 1); // 로그인서버 실행여부 ON / OFF
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, _process_total_time.get_formatted_value(PDH_FMT_LARGE | PDH_FMT_NOCAP100).largeValue / _number_of_processor); // 로그인서버 CPU 사용률
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, _process_private_byte.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x100000); // 로그인서버 메모리 사용 MByte
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_LOGIN_SESSION, _session_array._size); // 로그인서버 세션 수 (컨넥션 수)
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, _update_tps); // 로그인서버 인증 처리 초당 횟수
		_InterlockedExchange(&_update_tps, 0);
		auto& view_pool = data_structure::_thread_local::memory_pool<view>::instance();
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, view_pool._stack._capacity);// 로그인서버 패킷풀 사용량
	};
	virtual bool on_accept_socket(system_component::socket_address_ipv4& socket_address) noexcept override {
		return true;
	};
	virtual void on_create_session(unsigned long long key) noexcept override {
	};
	virtual bool on_receive_session(unsigned long long key, view_pointer& message_ptr) noexcept override {
		unsigned short type_;
		*message_ptr >> type_;
		if (!*message_ptr)
			return false;
		switch (type_) {
		case en_PACKET_CS_LOGIN_REQ_LOGIN: {
			if (72 != message_ptr->size())
				return false;

			__int64 account_no;
			char session_key[65]{};
			*message_ptr >> account_no;
			message_ptr->peek(reinterpret_cast<unsigned char*>(session_key), 64);
			if (!*message_ptr)
				return false;

			auto& mysql = design_pattern::_thread_local::singleton<database::mysql>::instance();
			mysql.query("SELECT `accountno`, `userid`, `usernick`, `sessionkey` FROM `v_account` WHERE `accountno` = % d", account_no);
			auto result = mysql.store_result();
			if (1 != result.num_row())
				return false;
			auto row = result.fetch_row();
			wchar_t id[20];
			wchar_t nickname[20];
			MultiByteToWideChar(CP_UTF8, 0, row.get_char_pointer(1), -1, id, 20);
			MultiByteToWideChar(CP_UTF8, 0, row.get_char_pointer(2), -1, nickname, 20);

			auto& redis = design_pattern::_thread_local::singleton<database::redis>::instance();
			char account_no_char[21]{};
			sprintf_s<21>(account_no_char, "%llu", account_no);
			redis.setex(account_no_char, 30, session_key);
			redis.sync_commit();

			auto msg_ptr = create_message(159);
			*msg_ptr << (unsigned short)en_PACKET_CS_LOGIN_RES_LOGIN << account_no << (unsigned char)1;
			msg_ptr->push((unsigned char*)id, 40);
			msg_ptr->push((unsigned char*)nickname, 40);
			auto session_ip = do_get_session_ip(key);
			if (!session_ip)
				return false;
			msg_ptr->push((unsigned char*)L"0.0.0.0", 32);
			*msg_ptr << (unsigned short)10403;
			if (L"127.0.0.1" == (*session_ip))
				msg_ptr->push((unsigned char*)L"127.0.0.1", 32);
			else if (L"10.0.1.2" == (*session_ip))
				msg_ptr->push((unsigned char*)L"10.0.1.1", 32);
			else if (L"10.0.2.2" == (*session_ip))
				msg_ptr->push((unsigned char*)L"10.0.2.1", 32);
			else
				msg_ptr->push((unsigned char*)L"106.245.38.108", 32);
			*msg_ptr << (unsigned short)10403;

			do_send_session(key, msg_ptr);
			_InterlockedIncrement(&_update_tps);
		} break;
		default:
			return false;
		}
		return true;
	};
	virtual void on_destroy_session(unsigned long long key) noexcept override {
	};
	virtual void on_destory_group(unsigned long long key) noexcept override {
	};

	inline void monitor_data_update_send(unsigned long long session_key, unsigned char data_type, int data_vaule) noexcept {
		system_component::time::unix unix;
		unix.time();
		int time_stamp = (int)unix.data();

		auto msg_ptr = create_message(11);
		*msg_ptr << (unsigned short)en_PACKET_SS_MONITOR_DATA_UPDATE << data_type << data_vaule << time_stamp;
		do_send_session(session_key, msg_ptr);
	}

	unsigned long long _monitor_server;
	unsigned long _update_tps = 0;
};

