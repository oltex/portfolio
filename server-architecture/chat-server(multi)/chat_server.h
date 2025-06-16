#pragma once
#include "server.h"
#include "library/design-pattern/singleton.h"
#include "library/data-structure/intrusive/list.h"
#include "library/system-component/lock/slim_read_write.h"
#include <array>

#include "CommonProtocol.h"

using namespace system_component;
using namespace data_structure;

class chat_server final : public server, public design_pattern::singleton<chat_server> {
private:
	using size_type = unsigned int;
	friend class design_pattern::singleton<chat_server>;
	class user final : public data_structure::intrusive::list_hook<0> {
	public:
		inline explicit user(unsigned long long key) noexcept
			: _key(key) {
		};
		inline explicit user(user&) noexcept = delete;
		inline explicit user(user&&) noexcept = delete;
		inline auto operator=(user&) noexcept -> user & = delete;
		inline auto operator=(user&&) noexcept -> user & = delete;
		inline ~user(void) noexcept = default;
	public:
		unsigned long long _key;
		bool _login = false;
		unsigned short _sector_x = 0;
		unsigned short _sector_y = 0;

		u_int64 _account_no = 0;
		wchar_t _id[20];
		wchar_t _nick_name[20];
		char _session_key[65]{};
	};
	class sector final {
	public:
		inline explicit sector(void) noexcept = default;
		inline explicit sector(sector const& rhs) noexcept = default;
		inline explicit sector(sector&& rhs) noexcept = default;
		inline auto operator=(sector const& rhs) noexcept -> sector&;
		inline auto operator=(sector&& rhs) noexcept -> sector&;
		inline ~sector(void) noexcept = default;
	public:
		inline void insert(user& user_, unsigned short x, unsigned short y) noexcept {
			x += 1;
			y += 1;
			user_._sector_x = x;
			user_._sector_y = y;
			_srwlock[y][x].acquire_exclusive();
			_sector[y][x].push_back(user_);
			_srwlock[y][x].release_exclusive();
		}
		inline void erase(user& user_) noexcept {
			if (user_._sector_x != 0) {
				_srwlock[user_._sector_y][user_._sector_x].acquire_exclusive();
				_sector[user_._sector_y][user_._sector_x].erase(user_);
				_srwlock[user_._sector_y][user_._sector_x].release_exclusive();
			}
		}
		inline void move(user& user_, unsigned short x, unsigned short y) noexcept {
			if (0 == user_._sector_x) {
				insert(user_, x, y);
				return;
			}
			x += 1;
			y += 1;
			unsigned short user_x = user_._sector_x;
			unsigned short user_y = user_._sector_y;
			unsigned short first_x;
			unsigned short first_y;
			unsigned short second_x;
			unsigned short second_y;

			if (user_y * 52 + user_x == y * 52 + x) {
				return;
			}
			else if (user_y * 52 + user_x < y * 52 + x) {
				first_x = user_x;
				first_y = user_y;
				second_x = x;
				second_y = y;
			}
			else if (user_y * 52 + user_x > y * 52 + x) {
				first_x = x;
				first_y = y;
				second_x = user_x;
				second_y = user_y;
			}

			_srwlock[first_y][first_x].acquire_exclusive();
			_srwlock[second_y][second_x].acquire_exclusive();

			_sector[user_y][user_x].erase(user_);
			user_._sector_x = x;
			user_._sector_y = y;
			_sector[y][x].push_back(user_);

			_srwlock[first_y][first_x].release_exclusive();
			_srwlock[second_y][second_x].release_exclusive();
		}
		inline auto get(user& user_) noexcept ->
			std::pair<std::array<data_structure::intrusive::list<user, 0>*, 9>, std::array<system_component::lock::slim_read_write*, 9>> {
			std::pair<std::array<data_structure::intrusive::list<user, 0>*, 9>, std::array<system_component::lock::slim_read_write*, 9>> result{};
			size_type index = 0;
			for (unsigned short y = user_._sector_y - 1; y <= user_._sector_y + 1; ++y)
				for (unsigned short x = user_._sector_x - 1; x <= user_._sector_x + 1; ++x) {
					result.first[index] = &_sector[y][x];
					_srwlock[y][x].acquire_shared();
					result.second[index++] = &_srwlock[y][x];
				}
			return result;
		}
	private:
		intrusive::list<user, 0> _sector[52][52];
		lock::slim_read_write _srwlock[52][52];
	};
private:
	inline explicit chat_server(void) noexcept = default;
	inline explicit chat_server(chat_server const& rhs) noexcept = delete;
	inline explicit chat_server(chat_server&& rhs) noexcept = delete;
	inline auto operator=(chat_server const& rhs) noexcept -> chat_server & = delete;
	inline auto operator=(chat_server&& rhs) noexcept -> chat_server & = delete;
	inline ~chat_server(void) noexcept = default;
public:
	virtual void on_start(void) noexcept override {
		//_monitor_server = do_create_session("127.0.0.1", 10405);
		//auto message_ptr = create_message(6);
		//*message_ptr << (unsigned short)en_PACKET_SS_MONITOR_LOGIN << (int)2;
		//do_send_session(_monitor_server, message_ptr);
	};
	virtual void on_stop(void) noexcept override {
	};
	virtual void on_worker_start(void) noexcept {
		auto& redis = design_pattern::_thread_local::singleton<database::redis>::instance();
		//redis.connect("127.0.0.1", 6379, nullptr, 0, 0, 0);
		redis.connect("10.0.2.2", 6379, nullptr, 0, 0, 0);
	}
	virtual void on_monit(void) noexcept override {
		auto& user_pool = data_structure::_thread_local::memory_pool<user>::instance();
		printf("--------------------------------------\n"\
			"[ Content Monitor ]\n"\
			"Connect\n"\
			" Player Count   :   %llu\n"\
			"Traffic\n"\
			" Job Update      :   %u TPS\n"\
			"Resource\n"\
			" User   - Pool Count :   %u\n"\
			"          Use Count  :   %u\n",
			_user_map.size(), job_update_tps,
			user_pool._stack._capacity, user_pool._use_count);


		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1);// 채팅서버 ChatServer 실행 여부 ON / OFF
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, _process_total_time.get_formatted_value(PDH_FMT_LARGE | PDH_FMT_NOCAP100).largeValue / _number_of_processor); // 채팅서버 ChatServer CPU 사용률
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, _process_private_byte.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x100000); // 채팅서버 ChatServer 메모리 사용 MByte
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SESSION, _session_array._size);// 채팅서버 세션 수 (컨넥션 수)
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_PLAYER, _auth_success);// 채팅서버 인증성공 사용자 수 (실제 접속자)
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, job_update_tps);// 채팅서버 UPDATE 스레드 초당 초리 횟수
		_InterlockedExchange(&job_update_tps, 0);
		//auto& view_pool = data_structure::_thread_local::memory_pool<view>::instance();
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, view_pool._stack._capacity);// 채팅서버 패킷풀 사용량
		//monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, job_pool._stack._capacity); // 채팅서버 UPDATE MSG 풀 사용량
	};
	virtual bool on_accept_socket(system_component::socket_address_ipv4& socket_address) noexcept override {
		return true;
	};
	virtual void on_create_session(unsigned long long key) noexcept override {
		auto& memory_pool = data_structure::_thread_local::memory_pool<user>::instance();
		user* user_ = &memory_pool.allocate(key);

		_user_map_lock.acquire_exclusive();
		_user_map.emplace(key, user_);
		_user_map_lock.release_exclusive();

		_InterlockedIncrement(&job_update_tps);
	};
	virtual bool on_receive_session(unsigned long long key, view_pointer& view_ptr) noexcept override {
		unsigned short type__;
		*view_ptr >> type__;
		if (!*view_ptr)
			return false;
		switch (static_cast<en_PACKET_TYPE>(type__)) {
		case en_PACKET_CS_CHAT_REQ_LOGIN: {
			if (152 != view_ptr->size())
				return false;
			_user_map_lock.acquire_shared();
			user* user_ = _user_map.find(key)->second;
			_user_map_lock.release_shared();
			if (true == user_->_login)
				return false;

			*view_ptr >> user_->_account_no;
			view_ptr->peek(reinterpret_cast<unsigned char*>(user_->_id), 40);
			view_ptr->pop(40);
			view_ptr->peek(reinterpret_cast<unsigned char*>(user_->_nick_name), 40);
			view_ptr->pop(40);
			view_ptr->peek(reinterpret_cast<unsigned char*>(user_->_session_key), 64);
			view_ptr->pop(64);

			auto& redis = design_pattern::_thread_local::singleton<database::redis>::instance();
			char account_no_char[21]{};
			sprintf_s<21>(account_no_char, "%llu", user_->_account_no);
			auto fucture = redis.get(account_no_char);
			redis.del(std::vector<std::string>{ account_no_char });
			redis.sync_commit();

			auto reply = fucture.get();
			if (reply.is_null() || !reply.is_string() ||
				0 != strcmp(reply.as_string().c_str(), user_->_session_key)) {
				return false;
			}

			user_->_login = true;
			do_set_timeout_session(key, 40000);

			auto message_ptr = create_message(11);
			*message_ptr << (unsigned short)en_PACKET_CS_CHAT_RES_LOGIN << (unsigned char)true << user_->_account_no;
			do_send_session(user_->_key, message_ptr);
		}break;
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE: {
			if (12 != view_ptr->size())
				return false;
			_user_map_lock.acquire_shared();
			user* user_ = _user_map.find(key)->second;
			_user_map_lock.release_shared();

			#pragma region skip
			if (false == user_->_login)
				return false;
			unsigned __int64 account_no;
			unsigned short sector_x;
			unsigned short sector_y;
			*view_ptr >> account_no >> sector_x >> sector_y;
			view_ptr.get();
			if (account_no != user_->_account_no || 50 < sector_x || 50 < sector_y)
				return false;
#pragma endregion

			_sector.move(*user_, sector_x, sector_y);
			auto message_ptr = create_message(14);
			*message_ptr << (unsigned short)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << user_->_account_no << sector_x << sector_y;
			do_send_session(user_->_key, message_ptr);
		} break;
		case en_PACKET_CS_CHAT_REQ_MESSAGE: {
			if (256 < view_ptr->size())
				return false;
			_user_map_lock.acquire_shared();
			user* user_ = _user_map.find(key)->second;
			_user_map_lock.release_shared();
			if (false == user_->_login)
				return false;
			unsigned __int64 account_no;
			unsigned short message_length;
			*view_ptr >> account_no >> message_length;
			if (account_no != user_->_account_no)
				return false;
			wchar_t message_buffer[256]{};
			view_ptr->peek((unsigned char*)message_buffer, message_length);
			if (!*view_ptr)
				return false;
			view_ptr->pop(message_length);

			auto message_ptr = create_message(256);
			*message_ptr << (unsigned short)en_PACKET_CS_CHAT_RES_MESSAGE << user_->_account_no;
			message_ptr->push((unsigned char*)user_->_id, 40);
			message_ptr->push((unsigned char*)user_->_nick_name, 40);
			*message_ptr << message_length;
			message_ptr->push((unsigned char*)message_buffer, message_length);

			auto [area, srw] = _sector.get(*user_);
			for (auto index = 0; index < 9 && area[index] != nullptr; ++index) {
				for (auto& iter : *area[index])
					do_send_session(iter._key, message_ptr);
				srw[index]->release_shared();
			}
		} break;
		case en_PACKET_CS_CHAT_REQ_HEARTBEAT: {
		} break;
		default: {
			return false;
		}
		}
		_InterlockedIncrement(&job_update_tps);
		return true;
	};
	virtual void on_destroy_session(unsigned long long key) noexcept override {
		auto& memory_pool = data_structure::_thread_local::memory_pool<user>::instance();
		_user_map_lock.acquire_exclusive();
		auto iter = _user_map.find(key);
		if (iter == _user_map.end())
			__debugbreak();
		user* user_ = iter->second;
		_user_map.erase(iter);
		_user_map_lock.release_exclusive();

		if (user_->_sector_x != 0)
			_sector.erase(*user_);
		memory_pool.deallocate(*user_);
		_InterlockedIncrement(&job_update_tps);
	};
	virtual void on_destory_group(unsigned long long key) noexcept override {
	}

	inline void monitor_data_update_send(unsigned long long session_key, unsigned char data_type, int data_vaule) noexcept {
		system_component::time::unix unix;
		unix.time();
		int time_stamp = (int)unix.data();

		auto message_ptr = create_message(11);
		*message_ptr << (unsigned short)en_PACKET_SS_MONITOR_DATA_UPDATE << data_type << data_vaule << time_stamp;
		do_send_session(session_key, message_ptr);

	}
public:
	unsigned long long _monitor_server;

	std::unordered_map<unsigned long long, user*> _user_map;
	lock::slim_read_write _user_map_lock;

	sector _sector;

	size_type _auth_success = 0;
	size_type job_update_tps = 0;
};