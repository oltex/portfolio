#pragma once
#include "server.h"
#include "MonitorProtocol.h"
#include "library/system-component/lock/slim_read_write.h"
#include <unordered_set>

class monitor_server final : public server {
	struct monitor_data final {
		system_component::lock::slim_read_write _lock;
		unsigned long long sum = 0;
		unsigned int cnt = 0;
		int min = INT_MAX;
		int max = 0;
	};
public:
	inline explicit monitor_server(void) noexcept = default;
	inline explicit monitor_server(monitor_server const&) noexcept = delete;
	inline explicit monitor_server(monitor_server&&) noexcept = delete;
	inline auto operator=(monitor_server const&) noexcept -> monitor_server & = delete;
	inline auto operator=(monitor_server&&) noexcept -> monitor_server & = delete;
	inline ~monitor_server(void) noexcept = default;

	inline virtual void on_start(void) noexcept override {
		_mysql.connect("10.0.1.2", "root", "1111", "monitorlog", 3306);
		do_create_function(&monitor_server::database_save, this);

		auto& query = utility::performance_data_helper::query::instance();
		__processor_total_time = query.add_counter(L"\\Processor(_Total)\\% Processor Time");
		__memory_pool_nonpaged_byte = query.add_counter(L"\\Memory\\Pool Nonpaged Bytes");
		__tcpv4_segments_received_sec = query.add_counter(L"\\TCPv4\\Segments Received/sec");
		__tcpv4_segments_sent_sec = query.add_counter(L"\\TCPv4\\Segments Sent/sec");
		__memory_available_byte = query.add_counter(L"\\Memory\\Available Bytes");
	}
	inline virtual void on_worker_start(void) noexcept override {
	}
	inline virtual void on_stop(void) noexcept override {
	}
	inline virtual void on_monit(void) noexcept override {
		auto& query = utility::performance_data_helper::query::instance();
		system_component::time::unix unix;
		unix.time();

		unsigned char server_no = 3;
		int time_stamp = (int)unix.data();

		int processor_total_time_data = __processor_total_time.get_formatted_value(PDH_FMT_LONG).longValue;
		int memory_pool_nonpaged_byte_data = (int)(__memory_pool_nonpaged_byte.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x100000);
		int tcpv4_segments_received_sec_data = (int)(__tcpv4_segments_received_sec.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x400);
		int tcpv4_segments_sent_sec_data = (int)(__tcpv4_segments_sent_sec.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x400);
		int memory_available_byte_data = (int)(__memory_available_byte.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x100000);

		database_insert(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, processor_total_time_data);
		database_insert(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, memory_pool_nonpaged_byte_data);
		database_insert(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, processor_total_time_data);
		database_insert(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, processor_total_time_data);
		database_insert(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, processor_total_time_data);

		auto message_ptr0 = create_message(12);
		auto message_ptr1 = create_message(12);
		auto message_ptr2 = create_message(12);
		auto message_ptr3 = create_message(12);
		auto message_ptr4 = create_message(12);
		*message_ptr0 << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << server_no << (unsigned char)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << processor_total_time_data << time_stamp;
		*message_ptr1 << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << server_no << (unsigned char)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << memory_pool_nonpaged_byte_data << time_stamp;
		*message_ptr2 << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << server_no << (unsigned char)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << tcpv4_segments_received_sec_data << time_stamp;
		*message_ptr3 << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << server_no << (unsigned char)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << tcpv4_segments_sent_sec_data << time_stamp;
		*message_ptr4 << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << server_no << (unsigned char)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << memory_available_byte_data << time_stamp;

		_monitor_client_lock.acquire_shared();
		for (auto& iter : _monitor_client) {
			do_send_session(iter, message_ptr0);
			do_send_session(iter, message_ptr1);
			do_send_session(iter, message_ptr2);
			do_send_session(iter, message_ptr3);
			do_send_session(iter, message_ptr4);
		}
		_monitor_client_lock.release_shared();
	}
	inline virtual bool on_accept_socket(system_component::socket_address_ipv4& socket_address) noexcept override {
		return true;
	}
	inline virtual void on_create_session(unsigned long long key) noexcept override {
	}
	inline virtual bool on_receive_session(unsigned long long key, view_pointer& view_ptr) noexcept override {
		unsigned short type_;
		*view_ptr >> type_;
		if (!*view_ptr)
			return false;
		switch (type_) {
		case en_PACKET_SS_MONITOR_LOGIN: {
			if (4 != view_ptr->size())
				return false;

			auto ip = do_get_session_ip(key);
			if (!ip || ip != L"127.0.0.1")
				return false;

			int server_no;
			*view_ptr >> server_no;
			if (3 < server_no)
				return false;
			if (0 != _other_server[server_no])
				return false;
			_other_server[server_no] = key;
		} break;
		case en_PACKET_SS_MONITOR_DATA_UPDATE: {
			if (9 != view_ptr->size())
				return false;

			unsigned char server_no = -1;
			unsigned char data_type;
			int data_value;
			int time_stamp;
			*view_ptr >> data_type >> data_value >> time_stamp;
			if (1 <= data_type && 6 >= data_type)
				server_no = 0;
			else if (10 <= data_type && 23 >= data_type)
				server_no = 1;
			else if (30 <= data_type && 37 >= data_type)
				server_no = 2;
			if (-1 == server_no || _other_server[server_no] != key)
				return false;

			database_insert(data_type, data_value);

			auto message_ptr = create_message(12);
			*message_ptr << (unsigned short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << server_no << data_type << data_value << time_stamp;
			_monitor_client_lock.acquire_shared();
			for (auto& iter : _monitor_client)
				do_send_session(iter, message_ptr);
			_monitor_client_lock.release_shared();
		} break;
		case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN: {
			if (32 != view_ptr->size())
				return false;
			char login_session_key[32]{};
			view_ptr->peek((unsigned char*)login_session_key, 32);
			view_ptr->pop(32);
			if (0 != strncmp(_login_session_key, login_session_key, 32))
				return false;
			_monitor_client_lock.acquire_exclusive();
			_monitor_client.emplace(key);
			_monitor_client_lock.release_exclusive();

			auto message_ptr = create_message(3);
			*message_ptr << (unsigned short)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN << (unsigned char)dfMONITOR_TOOL_LOGIN_OK;
		} break;
		default:
			return false;
		}
		return true;
	}
	inline virtual void on_destroy_session(unsigned long long key) noexcept override {
		for (int index = 0; index < 3; ++index) {
			if (_other_server[index] == key)
				_other_server[index] = 0;
		}
		_monitor_client_lock.acquire_exclusive();
		auto iter = _monitor_client.find(key);
		if (iter != _monitor_client.end())
			_monitor_client.erase(iter);
		_monitor_client_lock.release_exclusive();
	}
	inline virtual void on_destory_group(unsigned long long key) noexcept override {
	}

	inline int database_save(void) noexcept {
		system_component::time::unix unix;
		unix.time();
		system_component::time::date date(unix.local_time());

		_mysql.begin();
		_mysql.query("CREATE TABLE IF NOT EXISTS logdb%d%02d LIKE logdb;", date.year(), date.month());

		for (auto index = 0; index < 50; ++index) {
			if (0 != _monitor_data[index].cnt) {
				_monitor_data[index]._lock.acquire_exclusive();
				int server_no;
				if (1 <= index && 6 >= index)
					server_no = 0;
				else if (10 <= index && 23 >= index)
					server_no = 1;
				else if (30 <= index && 37 >= index)
					server_no = 2;
				else if (40 <= index && 44 >= index)
					server_no = 3;

				_mysql.query("INSERT INTO logdb%d%02d (logtime, serverno, type, avr, min, max) VALUES (NOW(), %d, %d, %d, %d, %d)",
					date.year(), date.month(), server_no, index, _monitor_data[index].sum / _monitor_data[index].cnt, _monitor_data[index].min, _monitor_data[index].max);

				_monitor_data[index].sum = 0;
				_monitor_data[index].cnt = 0;
				_monitor_data[index].min = INT_MAX;
				_monitor_data[index].max = 0;

				_monitor_data[index]._lock.release_exclusive();
			}
		}
		_mysql.commit();
		return 600000;
	}
	inline void database_insert(unsigned char data_type, int data_value) noexcept {
		_monitor_data[data_type]._lock.acquire_exclusive();
		_monitor_data[data_type].sum += data_value;
		if (_monitor_data[data_type].min > data_value)
			_monitor_data[data_type].min = data_value;
		if (_monitor_data[data_type].max < data_value)
			_monitor_data[data_type].max = data_value;
		_monitor_data[data_type].cnt++;
		_monitor_data[data_type]._lock.release_exclusive();
	}

	unsigned long long _other_server[3]{ 0, 0, 0 };
	std::unordered_set<unsigned long long> _monitor_client;
	system_component::lock::slim_read_write _monitor_client_lock;

	char _login_session_key[33] = "ajfw@!cv980dSZ[fje#@fdj123948djf";
	utility::performance_data_helper::query::counter __processor_total_time;
	utility::performance_data_helper::query::counter __memory_pool_nonpaged_byte;
	utility::performance_data_helper::query::counter __tcpv4_segments_received_sec;
	utility::performance_data_helper::query::counter __tcpv4_segments_sent_sec;
	utility::performance_data_helper::query::counter __memory_available_byte;

	database::mysql _mysql;
	monitor_data _monitor_data[50];
};