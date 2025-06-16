#pragma once
#include "server.h"
#include "library/design-pattern/singleton.h"
#include "library/data-structure/intrusive/list.h"
#include <array>

#include "CommonProtocol.h"

using namespace data_structure;
using namespace system_component;

class login_chat_server final : public server, public design_pattern::singleton<login_chat_server> {
private:
	using size_type = unsigned int;
	friend class design_pattern::singleton<login_chat_server>;
	enum class type : unsigned short {
		create_session = 1, receive_session, response_login, destory_session, close_update
	};
	struct job final : public data_structure::intrusive::shared_pointer_hook<0> {
		inline explicit job(void) noexcept = default;
		inline explicit job(job&) noexcept = delete;
		inline explicit job(job&&) noexcept = delete;
		inline auto operator=(job&) noexcept -> job & = delete;
		inline auto operator=(job&&) noexcept -> job & = delete;
		inline ~job(void) noexcept = default;
		inline void destructor(void) noexcept {
			auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
			memory_pool.deallocate(*this);
		}
		unsigned long long _key;
		type _type;
		view_pointer _message_ptr;
		cpp_redis::reply _reply;
	};
	using job_pointer = data_structure::intrusive::shared_pointer<job, 0>;
	class job_queue final : protected data_structure::lockfree::queue<job*> {
	private:
		using base = data_structure::lockfree::queue<job*>;
	public:
		inline explicit job_queue(void) noexcept = default;
		inline explicit job_queue(job_queue const&) noexcept = default;
		inline explicit job_queue(job_queue&&) noexcept = default;
		inline auto operator=(job_queue const&) noexcept -> job_queue&;
		inline auto operator=(job_queue&&) noexcept -> job_queue&;
		inline ~job_queue(void) noexcept = default;
	public:
		inline void push(job_pointer job_ptr) noexcept {
			_InterlockedIncrement(&_size);
			base::emplace(job_ptr.get());
			job_ptr.reset();
			system_component::wait_on_address::wake_single((void*)&_size);
		}
		inline auto pop(void) noexcept -> job_pointer {
			unsigned long long head = _head;
			node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
			unsigned long long next = address->_next;

			if (0x10000 > (0x00007FFFFFFFFFFFULL & next))
				__debugbreak();
			unsigned long long tail = _tail;
			if (tail == head)
				_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);

			job_pointer result;
			result.set(reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & next)->_value);
			_head = next;
			_memory_pool::instance().deallocate(*address);
			_InterlockedDecrement(&_size);
			return result;
		}
		inline auto empty(void) const noexcept {
			unsigned long long head = _head;
			node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
			unsigned long long next = address->_next;
			if (_nullptr == (0x00007FFFFFFFFFFFULL & next))
				return true;
			return false;
		}
		inline void wait(void) noexcept {
			long _compare = 0;
			system_component::wait_on_address::wait((void*)&_size, &_compare, sizeof(long), INFINITE);
		}
	public:
		volatile long _size = 0;
	};
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
			_sector[y + 1][x + 1].push_back(user_);
			user_._sector_x = x + 1;
			user_._sector_y = y + 1;
		}
		inline void erase(user& user_) noexcept {
			_sector[user_._sector_y][user_._sector_x].erase(user_);
		}
		inline auto get(user& user_) noexcept -> std::array<data_structure::intrusive::list<user, 0>*, 9> {
			std::array<data_structure::intrusive::list<user, 0>*, 9> result{};
			size_type index = 0;
			for (unsigned short y = user_._sector_y - 1; y <= user_._sector_y + 1; ++y)
				for (unsigned short x = user_._sector_x - 1; x <= user_._sector_x + 1; ++x)
					result[index++] = &_sector[y][x];
			return result;
		}
	private:
		data_structure::intrusive::list<user, 0> _sector[52][52];
	};
private:
	inline explicit login_chat_server(void) noexcept = default;
	inline explicit login_chat_server(login_chat_server const& rhs) noexcept = delete;
	inline explicit login_chat_server(login_chat_server&& rhs) noexcept = delete;
	inline auto operator=(login_chat_server const& rhs) noexcept -> login_chat_server & = delete;
	inline auto operator=(login_chat_server&& rhs) noexcept -> login_chat_server & = delete;
	inline ~login_chat_server(void) noexcept = default;
public:
	inline void update(void) noexcept {
		database::redis redis;
		//redis.connect("127.0.0.1", 6379, nullptr, 0, 0, 0);
		redis.connect("10.0.2.2", 6379, nullptr, 0, 0, 0);
		for (;;) {
			_job_queue.wait();
			while (!_job_queue.empty()) {
				auto job_ptr = _job_queue.pop();
				switch (job_ptr->_type) {
				case type::close_update:
					return;
				case type::create_session: {
					auto& memory_pool = data_structure::_thread_local::memory_pool<user>::instance();
					user* user_ = &memory_pool.allocate(job_ptr->_key);
					_user_map.emplace(job_ptr->_key, user_);
				} break;
				case type::response_login: {
					auto iter = _user_map.find(job_ptr->_key);
					if (iter == _user_map.end())
						break;
					user* user_ = iter->second;
					if (true == user_->_login) {
						do_destroy_session(job_ptr->_key);
						break;
					}
					if (job_ptr->_reply.is_null() || !job_ptr->_reply.is_string() || 
						0 != strcmp(job_ptr->_reply.as_string().c_str(), user_->_session_key)) {
						do_destroy_session(job_ptr->_key);
						break;
					}
					user_->_login = true;
					do_set_timeout_session(job_ptr->_key, 40000);
					auto message_ptr = create_message(11);
					*message_ptr << (unsigned short)en_PACKET_CS_CHAT_RES_LOGIN << (unsigned char)true << user_->_account_no;
					do_send_session(user_->_key, message_ptr);
				} break;
				case type::receive_session: {
					auto& message_ptr = job_ptr->_message_ptr;
					unsigned short type_;
					*message_ptr >> type_;
					if (!*message_ptr) {
						do_destroy_session(job_ptr->_key);
						break;
					}
					switch (static_cast<en_PACKET_TYPE>(type_)) {
					case en_PACKET_CS_CHAT_REQ_LOGIN: {
						if (152 != message_ptr->size()) {
							do_destroy_session(job_ptr->_key);
							break;
						}
						user* user_ = _user_map.find(job_ptr->_key)->second;
						if (true == user_->_login) {
							do_destroy_session(job_ptr->_key);
							break;
						}

						*message_ptr >> user_->_account_no;
						message_ptr->peek(
							reinterpret_cast<unsigned char*>(user_->_session_key), 64);
						message_ptr->pop(64);

						char account_no_char[21]{};
						sprintf_s<21>(account_no_char, "%llu", user_->_account_no);
						redis.get(account_no_char, 
							[key = user_->_key, &job_queue_ = _job_queue]
							(cpp_redis::reply& reply) {
							auto& pool = _thread_local::memory_pool<job>::instance();
							job_pointer job_ptr(&pool.allocate());
							job_ptr->_type = type::response_login;
							job_ptr->_key = key;
							job_ptr->_reply = reply;
							job_queue_.push(job_ptr);
							});
						redis.del(std::vector<std::string>{ account_no_char });
						redis.commit();
					}break;
					case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE: {
						if (12 != message_ptr->size()) {
							do_destroy_session(job_ptr->_key);
							break;
						}
						user* user_ = _user_map.find(job_ptr->_key)->second;
						if (false == user_->_login) {
							do_destroy_session(job_ptr->_key);
							break;
						}

						unsigned __int64 account_no;
						unsigned short sector_x;
						unsigned short sector_y;
						*message_ptr >> account_no >> sector_x >> sector_y;
						message_ptr.get();
						if (account_no != user_->_account_no || 50 < sector_x || 50 < sector_y) {
							do_destroy_session(job_ptr->_key);
							break;
						}

						if (0 != user_->_sector_x)
							_sector.erase(*user_);
						_sector.insert(*user_, sector_x, sector_y);

						view_pointer message_ptr = create_message(14);
						*message_ptr << (unsigned short)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << user_->_account_no << sector_x << sector_y;
						do_send_session(user_->_key, message_ptr);
					} break;
					case en_PACKET_CS_CHAT_REQ_MESSAGE: {
						if (256 < message_ptr->size()) {
							do_destroy_session(job_ptr->_key);
							break;
						}
						user* user_ = _user_map.find(job_ptr->_key)->second;
						if (false == user_->_login || 0 == user_->_sector_x) {
							do_destroy_session(job_ptr->_key);
							break;
						}

						unsigned __int64 account_no;
						unsigned short message_length;
						*message_ptr >> account_no >> message_length;
						if (account_no != user_->_account_no) {
							do_destroy_session(job_ptr->_key);
							break;
						}
						wchar_t message_buffer[256]{};
						message_ptr->peek((unsigned char*)message_buffer, message_length);
						if (!*message_ptr) {
							do_destroy_session(job_ptr->_key);
							break;
						}
						message_ptr->pop(message_length);

						view_pointer message_ptr = create_message(256);
						*message_ptr << (unsigned short)en_PACKET_CS_CHAT_RES_MESSAGE << user_->_account_no;
						message_ptr->push((unsigned char*)user_->_id, 40);
						message_ptr->push((unsigned char*)user_->_nick_name, 40);
						*message_ptr << message_length;
						message_ptr->push((unsigned char*)message_buffer, message_length);

						auto result = _sector.get(*user_);
						for (auto index = 0; index < 9 && result[index] != nullptr; ++index)
							for (auto& iter : *result[index])
								do_send_session(iter._key, message_ptr);

					}break;
					case en_PACKET_CS_CHAT_REQ_HEARTBEAT: {
					} break;
					default:
						do_destroy_session(job_ptr->_key);
						break;
					}
				} break;
				case type::destory_session: {
					auto& memory_pool = data_structure::_thread_local::memory_pool<user>::instance();
					auto iter = _user_map.find(job_ptr->_key);
					if (iter == _user_map.end())
						__debugbreak();
					user* user_ = iter->second;
					if (true == user_->_login)
						--_auth_success;
					_user_map.erase(iter);

					if (user_->_sector_x != 0)
						_sector.erase(*user_);
					memory_pool.deallocate(*user_);
				} break;
				default:
					__debugbreak();
					break;
				}
				job_update_tps++;
			}
		}
	};
public:
	virtual void on_start(void) noexcept override {
		_monitor_server = do_create_session("127.0.0.1", 10405);
		auto message_ptr = create_message(6);
		*message_ptr << (unsigned short)en_PACKET_SS_MONITOR_LOGIN << (int)2;
		do_send_session(_monitor_server, message_ptr);

		_update_thread.begin(&login_chat_server::update, 0, this);
	};
	virtual void on_stop(void) noexcept override {
		auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
		job_pointer job_ptr(&memory_pool.allocate());
		job_ptr->_type = type::close_update;
		_job_queue.push(job_ptr);
		_update_thread.wait_for_single(INFINITE);
	};
	virtual void on_worker_start(void) noexcept {
	}
	virtual void on_monit(void) noexcept override {
		auto& user_pool = data_structure::_thread_local::memory_pool<user>::instance();
		auto& job_pool = data_structure::_thread_local::memory_pool<job>::instance();
		printf("--------------------------------------\n"\
			"[ Content Monitor ]\n"\
			"Connect\n"\
			" Player Count   :   %llu\n"\
			"Traffic\n"\
			" Job Queue Size  :   %u\n"\
			" Job Update      :   %u TPS\n"\
			"Resource\n"\
			" User   - Pool Count :   %u\n"\
			"          Use Count  :   %u\n"\
			" Job    - Pool Count :   %u\n"\
			"          Use Count  :   %u\n",
			_user_map.size(), _job_queue._size, job_update_tps,
			user_pool._stack._capacity, user_pool._use_count,
			job_pool._stack._capacity, job_pool._use_count);

		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1);// 채팅서버 ChatServer 실행 여부 ON / OFF
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, _process_total_time.get_formatted_value(PDH_FMT_LARGE | PDH_FMT_NOCAP100).largeValue / _number_of_processor); // 채팅서버 ChatServer CPU 사용률
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, _process_private_byte.get_formatted_value(PDH_FMT_LARGE).largeValue / 0x100000); // 채팅서버 ChatServer 메모리 사용 MByte
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_SESSION, _session_array._size);// 채팅서버 세션 수 (컨넥션 수)
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_PLAYER, _auth_success);// 채팅서버 인증성공 사용자 수 (실제 접속자)
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, job_update_tps);// 채팅서버 UPDATE 스레드 초당 초리 횟수
		auto& view_pool = data_structure::_thread_local::memory_pool<view>::instance();
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, view_pool._stack._capacity);// 채팅서버 패킷풀 사용량
		monitor_data_update_send(_monitor_server, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, job_pool._stack._capacity); // 채팅서버 UPDATE MSG 풀 사용량
		job_update_tps = 0;
	};
	virtual bool on_accept_socket(system_component::socket_address_ipv4& socket_address) noexcept override {
		return true;
	};
	virtual void on_create_session(unsigned long long key) noexcept override {
		auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
		job_pointer job_ptr(&memory_pool.allocate());
		job_ptr->_key = key;
		job_ptr->_type = type::create_session;
		_job_queue.push(job_ptr);
	};
	virtual bool on_receive_session(unsigned long long key, view_pointer& message_ptr) noexcept override {
		auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
		job_pointer job_ptr(&memory_pool.allocate());
		job_ptr->_key = key;
		job_ptr->_type = type::receive_session;
		job_ptr->_message_ptr = message_ptr;
		_job_queue.push(job_ptr);
		return true;
	};
	virtual void on_destroy_session(unsigned long long key) noexcept override {
		if (_monitor_server == key)
			return;
		auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
		job_pointer job_ptr(&memory_pool.allocate());
		job_ptr->_key = key;
		job_ptr->_type = type::destory_session;
		job_ptr->_message_ptr = nullptr;
		_job_queue.push(job_ptr);
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

	system_component::thread _update_thread;
	job_queue _job_queue;
	size_type job_update_tps = 0;
	std::unordered_map<unsigned long long, user*> _user_map;
	sector _sector;

	size_type _auth_success = 0;
};