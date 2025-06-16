#pragma once
#include "library/system-component/inputoutput_completion_port.h"
#include "library/system-component/thread.h"
#include "library/system-component/socket.h"
#include "library/system-component/wait_on_address.h"
#include "library/system-component/time/multimedia.h"

#include "library/data-structure/serialize_buffer.h"
#include "library/data-structure/thread-local/memory_pool.h"
#include "library/data-structure/intrusive/shared_pointer.h"
#include "library/data-structure/intrusive/list.h"
#include "library/data-structure/lockfree/queue.h"
#include "library/data-structure/priority_queue.h"

#include "library/database/mysql.h"
#include "library/database/redis.h"

#include "library/utility/command.h"
#include "library/utility/performance_data_helper.h"
#include "library/utility/logger.h"
#include "library/utility/crash_dump.h"

#include "library/design-pattern/singleton.h"

#include <optional>
#include <iostream>
#include <intrin.h>
#include <functional>

template<typename type>
concept string_size = std::_Is_any_of_v<type, unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long>;

class server {
private:
	using size_type = unsigned int;
	using byte = unsigned char;
	enum class post_queue_state {
		close_worker, destory_session, excute_task, destory_group
	};
	class session final {
	public:
#pragma pack(push, 1)
		struct header final {
			inline explicit header(void) noexcept = default;
			inline explicit header(header const&) noexcept = delete;
			inline explicit header(header&&) noexcept = delete;
			inline auto operator=(header const&) noexcept -> header & = delete;
			inline auto operator=(header&&) noexcept -> header & = delete;
			inline ~header(void) noexcept = default;
			unsigned char _code;
			unsigned short _length;
			unsigned char _random_key;
			unsigned char _check_sum;
		};
#pragma pack(pop)
		class message final : public data_structure::intrusive::shared_pointer_hook<0>, public data_structure::serialize_buffer<> {
		public:
			inline explicit message(void) noexcept = delete;
			inline explicit message(message const&) noexcept = delete;
			inline explicit message(message&&) noexcept = delete;
			inline auto operator=(message const&) noexcept -> message & = delete;
			inline auto operator=(message&&) noexcept -> message & = delete;
			inline ~message(void) noexcept = delete;

			inline void destructor(void) noexcept {
				auto& memory_pool = data_structure::_thread_local::memory_pool<message>::instance();
				memory_pool.deallocate(*this);
			}
		};
		using message_pointer = data_structure::intrusive::shared_pointer<message, 0>;
		class view final : public data_structure::intrusive::shared_pointer_hook<0> {
		private:
			using size_type = unsigned int;
		public:
			inline explicit view(void) noexcept
				: _front(0), _rear(0), _fail(false) {
			}
			inline explicit view(message_pointer message_, size_type front, size_type rear) noexcept
				: _message(message_), _front(front), _rear(rear), _fail(false) {
			}
			inline view(view const& rhs) noexcept
				: _message(rhs._message), _front(rhs._front), _rear(rhs._rear), _fail(rhs._fail) {
			}
			inline auto operator=(view const& rhs) noexcept -> view&;
			inline auto operator=(view&& rhs) noexcept -> view&;
			inline ~view(void) noexcept = default;

			template<typename type>
				requires std::is_arithmetic_v<type>
			inline auto operator<<(type const& value) noexcept -> view& {
				reinterpret_cast<type&>(_message->data()[_rear]) = value;
				_rear += sizeof(type);
				return *this;
			}
			inline void push(byte* const buffer, size_type const length) noexcept {
				memcpy(_message->data() + _rear, buffer, length);
				_rear += length;
			}
			template<typename type>
				requires std::is_arithmetic_v<type>
			inline auto operator>>(type& value) noexcept -> view& {
				if (sizeof(type) + _front > _rear) {
					_fail = true;
					return *this;
				}
				value = reinterpret_cast<type&>(_message->data()[_front]);
				_front += sizeof(type);
				return *this;
			}
			inline void peek(byte* const buffer, size_type const length) noexcept {
				if (length + _front > _rear) {
					_fail = true;
					return;
				}
				memcpy(buffer, _message->data() + _front, length);
			}
			inline void pop(size_type const length) noexcept {
				_front += length;
			}
			inline auto front(void) const noexcept -> size_type {
				return _front;
			}
			inline auto rear(void) const noexcept -> size_type {
				return _rear;
			}
			inline void move_front(size_type const length) noexcept {
				_front += length;
			}
			inline void move_rear(size_type const length) noexcept {
				_rear += length;
			}
			inline auto size(void) const noexcept -> size_type {
				return _rear - _front;
			}
			inline auto begin(void) noexcept {
				return _message->data() + _front;
			}
			inline auto end(void) noexcept {
				return _message->data() + _rear;
			}
			inline auto data(void) noexcept -> message_pointer& {
				return _message;
			}
			inline void set(message* message_, size_type front, size_type rear) noexcept {
				_front = front;
				_rear = rear;
				_fail = false;
				_message.set(message_);
			}
			inline auto reset(void) noexcept {
				_message.reset();
			}
			inline operator bool(void) const noexcept {
				return !_fail;
			}
			inline auto fail(void) const noexcept {
				return _fail;
			}

			inline void destructor(void) noexcept {
				auto& memory_pool = data_structure::_thread_local::memory_pool<view>::instance();
				memory_pool.deallocate(*this);
			}
		private:
			size_type _front, _rear;
			bool _fail = false;
			message_pointer _message;
		};
		using view_pointer = data_structure::intrusive::shared_pointer<view, 0>;
	private:
		class queue final : protected data_structure::lockfree::queue<view*> {
		private:
			using base = data_structure::lockfree::queue<view*>;
			class iterator final {
			public:
				inline explicit iterator(void) noexcept = default;
				inline explicit iterator(node* node_) noexcept
					: _node(node_) {
				}
				inline explicit iterator(iterator const&) noexcept = delete;
				inline explicit iterator(iterator&&) noexcept = delete;
				inline auto operator=(iterator const&) noexcept -> iterator & = delete;
				inline auto operator=(iterator&&) noexcept -> iterator & = delete;
				inline ~iterator(void) noexcept = default;

				inline auto operator*(void) const noexcept -> view*& {
					return _node->_value;
				}
				inline auto operator++(void) noexcept -> iterator& {
					_node = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & _node->_next);
					return *this;
				}
				inline bool operator==(iterator const& rhs) const noexcept {
					return _node == rhs._node;
				}
			private:
				node* _node;
			};
		public:
			inline explicit queue(void) noexcept = default;
			inline explicit queue(queue const&) noexcept = delete;
			inline explicit queue(queue&&) noexcept = delete;
			inline auto operator=(queue const&) noexcept -> queue & = delete;
			inline auto operator=(queue&&) noexcept -> queue & = delete;
			inline ~queue(void) noexcept = default;

			inline void push(view_pointer view_ptr) noexcept {
				base::emplace(view_ptr.get());
				view_ptr.reset();
				_InterlockedIncrement(&_size);
			}
			inline auto pop(void) noexcept -> view_pointer {
				unsigned long long head = _head;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				unsigned long long next = address->_next;

				if (0x10000 > (0x00007FFFFFFFFFFFULL & next))
					__debugbreak();
				unsigned long long tail = _tail;
				if (tail == head)
					_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);

				view_pointer result;
				result.set(reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & next)->_value);
				_head = next;
				_memory_pool::instance().deallocate(*address);

				_InterlockedDecrement(&_size);
				return result;
			}
			inline auto begin(void) noexcept -> iterator {
				return iterator(reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & _head)->_next));
			}
			inline auto end(void) noexcept -> iterator {
				return iterator(reinterpret_cast<node*>(_nullptr));
			}
			inline auto empty(void) const noexcept {
				unsigned long long head = _head;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				unsigned long long next = address->_next;
				if (_nullptr == (0x00007FFFFFFFFFFFULL & next))
					return true;
				return false;
			}
			inline auto size(void) const noexcept {
				return _size;
			}
		private:
			size_type _size = 0;
		};
		inline static unsigned long long _static_id = 0x10000;
	public:
#pragma warning(suppress: 26495)
		inline explicit session(size_type const index) noexcept
			: _key(index), _io_count(0x80000000) {
			auto& memory_pool = data_structure::_thread_local::memory_pool<message>::instance();
			message_pointer receive_message(&memory_pool.allocate());
			receive_message->clear();
			_receive_message = receive_message;
		};
		inline explicit session(session const&) noexcept = delete;
		inline explicit session(session&&) noexcept = delete;
		inline auto operator=(session const&) noexcept -> session & = delete;
		inline auto operator=(session&&) noexcept -> session & = delete;
		inline ~session(void) noexcept = default;

		inline void initialize(system_component::socket&& socket, unsigned long long timeout_duration) noexcept {
			_key = 0xffff & _key | _static_id;
			_static_id += 0x10000;
			_socket = std::move(socket);
			_timeout_currnet = GetTickCount64();
			_timeout_duration = timeout_duration;
			_send_flag = 0;
			_cancel_flag = 0;
			_InterlockedIncrement(&_io_count);
			_InterlockedAnd((long*)&_receive_count, 0x3FFFFFFF);
			_InterlockedAnd((long*)&_io_count, 0x7FFFFFFF);
		}
		inline auto acquire(void) noexcept -> bool {
			auto io_count = _InterlockedIncrement(&_io_count);
			if (0x80000000 & io_count)
				return false;
			return true;
		}
		inline auto acquire(unsigned long long key) noexcept -> bool {
			auto io_count = _InterlockedIncrement(&_io_count);
			if ((0x80000000 & io_count) || _key != key)
				return false;
			return true;
		}
		inline bool release(void) noexcept {
			if (0 == _InterlockedDecrement(&_io_count) && 0 == _InterlockedCompareExchange(&_io_count, 0x80000000, 0)) {
				_receive_message->clear();
				while (!_send_queue.empty())
					_send_queue.pop();
				while (!_receive_queue.empty())
					_receive_queue.pop();
				_socket.close();
				return true;
			}
			return false;
		}
		inline void receive(void) noexcept {
			if (0 == _cancel_flag) {
				WSABUF wsa_buffer{ message::capacity() - _receive_message->rear(),  reinterpret_cast<char*>(_receive_message->data() + _receive_message->rear()) };
				unsigned long flag = 0;
				_recv_overlapped.clear();
				_InterlockedIncrement(&_io_count);
				if (SOCKET_ERROR == _socket.wsa_receive(&wsa_buffer, 1, &flag, _recv_overlapped)) {
					if (WSA_IO_PENDING == GetLastError()) {
						if (1 == _cancel_flag)
							_socket.cancel_io_ex();
					}
					else {
						_InterlockedDecrement(&_io_count);
						_cancel_flag = 1;
					}
				}
			}
		}
		inline void send(void) noexcept {
			if (0 == _cancel_flag) {
				while (!_send_queue.empty() && 0 == _InterlockedExchange(&_send_flag, 1)) {
					if (_send_queue.empty())
						_InterlockedExchange(&_send_flag, 0);
					else {
						WSABUF wsa_buffer[512];
						_send_size = 0;
						for (auto iter = _send_queue.begin(), end = _send_queue.end(); iter != end; ++iter) {
							if (512 <= _send_size) {
								cancel();
								return;
							}
							wsa_buffer[_send_size].buf = reinterpret_cast<char*>((*iter)->data()->data() + (*iter)->front());
							wsa_buffer[_send_size].len = (*iter)->size();
							_send_size++;
						}
						_send_overlapped.clear();
						_InterlockedIncrement(&_io_count);
						if (SOCKET_ERROR == _socket.wsa_send(wsa_buffer, _send_size, 0, _send_overlapped)) {
							if (GetLastError() == WSA_IO_PENDING) {
								if (1 == _cancel_flag)
									_socket.cancel_io_ex();
							}
							else {
								_InterlockedDecrement(&_io_count);
								_cancel_flag = 1;
							}
						}
						return;
					}
				}
			}
		}
		inline void cancel(void) noexcept {
			_cancel_flag = 1;
			_socket.cancel_io_ex();
		}
		inline bool ready_receive(void) noexcept {
			if (_receive_message->size() != _receive_message->capacity()) {
				auto& memory_pool = data_structure::_thread_local::memory_pool<message>::instance();
				message_pointer receive_message(&memory_pool.allocate());
				receive_message->clear();
				if (0 < _receive_message->size()) {
					memcpy(receive_message->data(), _receive_message->data() + _receive_message->front(), _receive_message->size());
					receive_message->move_rear(_receive_message->size());
				}
				_receive_message = receive_message;
				return true;
			}
			cancel();
			//log_message("server", utility::logger::level::info, L"session(%llu) close / reason : receive buffer full");
			return false;
		}
		inline void finish_send(void) noexcept {
			for (size_type index = 0; index < _send_size; ++index)
				_send_queue.pop();
			_InterlockedExchange(&_send_flag, 0);
		}

		inline auto acquire_receive(void) noexcept -> bool {
			auto receive_count = _InterlockedIncrement(&_receive_count);
			if (0xC0000000 & receive_count)
				return false;
			return true;
		}
		inline bool release_receive(void) noexcept {
			if (0x40000000 == _InterlockedDecrement(&_receive_count) && 0x40000000 == _InterlockedCompareExchange(&_receive_count, 0xC0000000, 0x40000000))
				return true;
			return false;
		}
		inline bool group(void) noexcept {
			if (0x40000000 & _InterlockedOr((volatile long*)&_receive_count, (long)0x40000000))
				return false;
			return true;
		}

		unsigned long long _key;
		system_component::socket _socket;
		message_pointer _receive_message;
		queue _receive_queue;
		queue _send_queue;
		system_component::overlapped _recv_overlapped;
		system_component::overlapped _send_overlapped;
		unsigned long _io_count; // release_flag
		unsigned long _cancel_flag;
		unsigned long _receive_count; // group_flag, cancel_flag
		unsigned long _send_flag;
		unsigned long _send_size;
		unsigned long long _timeout_currnet;
		unsigned long long _timeout_duration;
		unsigned long long _group_key;
	};
	class session_array final {
	private:
		struct node final {
			inline explicit node(void) noexcept = delete;
			inline explicit node(node const&) noexcept = delete;
			inline explicit node(node&&) noexcept = delete;
			inline auto operator=(node const&) noexcept -> node & = delete;
			inline auto operator=(node&&) noexcept -> node & = delete;
			inline ~node(void) noexcept = delete;
			node* _next;
			session _value;
		};
		using iterator = node*;
	public:
#pragma warning(suppress: 26495)
		inline explicit session_array(void) noexcept = default;
		inline explicit session_array(session_array const&) noexcept = delete;
		inline explicit session_array(session_array&&) noexcept = delete;
		inline auto operator=(session_array const&) noexcept -> session_array & = delete;
		inline auto operator=(session_array&&) noexcept -> session_array & = delete;
		inline ~session_array(void) noexcept = default;

		inline void initialize(size_type const capacity) noexcept {
			_size = 0;
			_capacity = capacity;
			_array = reinterpret_cast<node*>(malloc(sizeof(node) * capacity));
			node* current = _array;
			node* next = current + 1;
			for (size_type index = 0; index < capacity - 1; ++index, current = next++) {
				current->_next = next;
				::new(reinterpret_cast<void*>(&current->_value)) session(index);
			}
#pragma warning(suppress: 6011)
			current->_next = nullptr;
			::new(reinterpret_cast<void*>(&current->_value)) session(capacity - 1);

			_head = reinterpret_cast<unsigned long long>(_array);
		}
		inline void finalize(void) noexcept {
			for (size_type index = 0; index < _capacity; ++index)
				_array[index]._value.~session();
			free(_array);
		}
		inline auto acquire(void) noexcept -> session* {
			for (;;) {
				unsigned long long head = _head;
				node* current = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				if (nullptr == current)
					return nullptr;
				unsigned long long next = reinterpret_cast<unsigned long long>(current->_next) + (0xFFFF800000000000ULL & head) + 0x0000800000000000ULL;
				if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head)) {
					_InterlockedIncrement(&_size);
					return &current->_value;
				}
			}
		}
		inline void release(session& value) noexcept {
			node* current = reinterpret_cast<node*>(reinterpret_cast<uintptr_t*>(&value) - 1);
			for (;;) {
				unsigned long long head = _head;
				current->_next = reinterpret_cast<node*>(head & 0x00007FFFFFFFFFFFULL);
				unsigned long long next = reinterpret_cast<unsigned long long>(current) + (head & 0xFFFF800000000000ULL);
				if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head))
					break;
			}
			_InterlockedDecrement(&_size);
		}
		inline auto begin(void) const noexcept -> iterator {
			return _array;
		}
		inline auto end(void) const noexcept -> iterator {
			return _array + _capacity;
		}
		inline auto operator[](unsigned long long const key) noexcept -> session& {
			return _array[key & 0xffff]._value;
		}
		inline void wait(void) const noexcept {
			while (_size != 0) {
			}
		}

		unsigned long long _head;
		node* _array;
		size_type _size;
		size_type _capacity;
	};
	class scheduler final {
	public:
		class task {
		public:
			enum class type : unsigned char {
				function, group
			};
#pragma warning(suppress: 26495)
			inline explicit task(type const type_) noexcept
				: _type(type_), _time(system_component::time::multimedia::get_time()) {
			};
			inline explicit task(task const&) noexcept = delete;
			inline explicit task(task&&) noexcept = delete;
			inline auto operator=(task const&) noexcept -> task & = delete;
			inline auto operator=(task&&) noexcept -> task & = delete;
			inline ~task(void) noexcept = default;

			inline virtual bool excute(void) noexcept = 0;

			type _type;
			unsigned long _time;
		};
		class function : public task {
		public:
			template <typename function_, typename... argument>
			inline explicit function(function_&& func, argument&&... arg) noexcept
				: task(type::function), _function(std::bind(std::forward<function_>(func), std::forward<argument>(arg)...)) {
			};
			inline explicit function(function const&) noexcept = delete;
			inline explicit function(function&&) noexcept = delete;
			inline auto operator=(function const&) noexcept -> function & = delete;
			inline auto operator=(function&&) noexcept -> function & = delete;
			inline ~function(void) noexcept = default;

			inline virtual bool excute(void) noexcept override {
				for (;;) {
					int time = _function();
					switch (time) {
					case 0:
						break;
					case -1:
						return false;
					default:
						_time += time;
						return true;
					}
				}
			}
		private:
			std::function<int(void)> _function;
		};
		class group : public task {
		private:
			friend class server;
			struct job final : public data_structure::intrusive::shared_pointer_hook<0> {
			public:
				enum class type : unsigned char {
					enter_session, leave_session, move_session
				};
				inline explicit job(void) noexcept = delete;
				inline explicit job(job&) noexcept = delete;
				inline explicit job(job&&) noexcept = delete;
				inline auto operator=(job&) noexcept -> job & = delete;
				inline auto operator=(job&&) noexcept -> job & = delete;
				inline ~job(void) noexcept = delete;

				inline void destructor(void) noexcept {
					auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
					memory_pool.deallocate(*this);
				}
				type _type;
				unsigned long long _session_key;
				unsigned long long _group_key;
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

				inline void push(job_pointer job_ptr) noexcept {
					base::emplace(job_ptr.get());
					job_ptr.reset();
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
			};
			inline static unsigned long long _static_id = 0x10000;
		public:
#pragma warning(suppress: 26495)
			inline explicit group(void) noexcept
				: task(type::group) {
				_key = _interlockedadd64((volatile long long*)&_static_id, 0x10000);
				_cancel_flag = 0;
				_InterlockedIncrement(&_io_count);
				_InterlockedAnd((long*)&_io_count, 0x7FFFFFFF);
			};
			inline explicit group(group const&) noexcept = delete;
			inline explicit group(group&&) noexcept = delete;
			inline auto operator=(group const&) noexcept -> group & = delete;
			inline auto operator=(group&&) noexcept -> group & = delete;
			inline ~group(void) noexcept {
				while (!_job_queue.empty())
					_job_queue.pop();
			};
		private:
			inline virtual bool excute(void) noexcept override {
				while (!_cancel_flag) {
					while (!_job_queue.empty()) {
						scheduler::group::job_pointer job_ptr = _job_queue.pop();
						switch (job_ptr->_type) {
						case scheduler::group::job::type::enter_session: {
							auto& session_ = _server->_session_array[job_ptr->_session_key];
							if (session_.acquire(job_ptr->_session_key)) {
								_session_map.emplace(job_ptr->_session_key, &session_);
								on_enter_session(job_ptr->_session_key);
								break;
							}
							if (session_.release()) {
								_server->on_destroy_session(session_._key);
								_server->_session_array.release(session_);
							}
						} break;
						case scheduler::group::job::type::leave_session: {
							auto iter = _session_map.find(job_ptr->_session_key);
							if (iter == _session_map.end())
								break;
							session& session_ = *iter->second;
							on_leave_session(session_._key);
							iter = _session_map.erase(iter);
							_InterlockedAnd((long*)&session_._receive_count, 0x3FFFFFFF);
							if (session_.release()) {
								_server->on_destroy_session(session_._key);
								_server->_session_array.release(session_);
							}
						} break;
						case scheduler::group::job::type::move_session: {
							auto iter = _session_map.find(job_ptr->_session_key);
							if (iter == _session_map.end())
								break;
							session& session_ = *iter->second;

							auto& group_ = _server->_group_array[job_ptr->_group_key];
							if (group_.acquire(job_ptr->_group_key)) {
								iter = _session_map.erase(iter);
								on_leave_session(session_._key);
								group_.do_enter_session(job_ptr->_session_key);
								if (session_.release()) {
									_server->on_destroy_session(session_._key);
									_server->_session_array.release(session_);
								}
							}
							if (group_.release()) {
								// temp
							}
						} break;
						default:
							__debugbreak();
						}
					}

					for (auto iter = _session_map.begin(), end = _session_map.end(); iter != end;) {
						session& session_ = *iter->second;
						if (!session_._cancel_flag) {
							while (!session_._receive_queue.empty()) {
								auto view_ptr = session_._receive_queue.pop();
								if (false == on_receive_session(session_._key, view_ptr)) {
									session_.cancel();
									break;
								}
							}
						}
						if (session_._cancel_flag) {
							on_leave_session(session_._key);
							iter = _session_map.erase(iter);
							if (session_.release()) {
								_server->on_destroy_session(session_._key);
								_server->_session_array.release(session_);
							}
						}
						else
							++iter;
					}

					int time = on_update();
					switch (time) {
					case 0:
						break;
					case -1:
						return false;
					default:
						_time += time;
						return true;
					}
				}
				return false;
			}
			inline auto acquire(unsigned long long key) noexcept -> bool {
				auto io_count = _InterlockedIncrement(&_io_count);
				if ((0x80000000 & io_count) || _key != key)
					return false;
				return true;
			}
			inline bool release(void) noexcept {
				if (0 == _InterlockedDecrement(&_io_count) && 0 == _InterlockedCompareExchange(&_io_count, 0x80000000, 0))
					return true;
				return false;
			}
			inline void cancel(void) noexcept {
				_cancel_flag = 1;
			}
			inline void do_enter_session(unsigned long long key) noexcept {
				auto& memory_pool = data_structure::_thread_local::memory_pool<job>::instance();
				job_pointer job_ptr(&memory_pool.allocate());
				job_ptr->_type = scheduler::group::job::type::enter_session;
				job_ptr->_session_key = key;
				_job_queue.push(job_ptr);
			}
			template<typename type>
			inline static void destructor(void* rhs) noexcept {
				auto& memory_pool = data_structure::_thread_local::memory_pool<type, 128, false>::instance();
				memory_pool.deallocate(*reinterpret_cast<type*>(rhs));
			}
		protected:
			inline virtual void on_monit(void) noexcept = 0;
			inline virtual void on_enter_session(unsigned long long key) noexcept = 0;
			inline virtual bool on_receive_session(unsigned long long key, session::view_pointer& view_ptr) noexcept = 0;
			inline virtual void on_leave_session(unsigned long long key) noexcept = 0;
			inline virtual int on_update(void) noexcept = 0;
			inline void do_leave_session(unsigned long long key) noexcept {
				auto& memory_pool = data_structure::_thread_local::memory_pool<group::job>::instance();
				job_pointer job_ptr(&memory_pool.allocate());
				job_ptr->_type = group::job::type::leave_session;
				job_ptr->_session_key = key;
				_job_queue.push(job_ptr);
			}
			inline void do_move_session(unsigned long long session_key, unsigned long long group_key) noexcept {
				auto& memory_pool = data_structure::_thread_local::memory_pool<group::job>::instance();
				job_pointer job_ptr(&memory_pool.allocate());
				job_ptr->_type = group::job::type::move_session;
				job_ptr->_session_key = session_key;
				job_ptr->_group_key = group_key;
				_job_queue.push(job_ptr);
			}
			inline void do_send_session(unsigned long long key, session::view_pointer& view_ptr) noexcept {
				_server->do_send_session(key, view_ptr);
			}
			inline void do_destroy_session(unsigned long long key) noexcept {
				_server->do_destroy_session(key);
			}
			inline void do_set_timeout_session(unsigned long long key, unsigned long long duration) noexcept {
				_server->do_set_timeout_session(key, duration);
			}
		private:
			unsigned long long _key;
			volatile unsigned int _io_count = 0; // release_flag // temp
			volatile unsigned int _cancel_flag;
			std::function<void(void*)> _destructor;
			server* _server;
			job_queue _job_queue;
			std::unordered_map<unsigned long long, session*> _session_map;
		};
		class group_array final {
		private:
			struct node final {
				inline explicit node(void) noexcept = delete;
				inline explicit node(node const&) noexcept = delete;
				inline explicit node(node&&) noexcept = delete;
				inline auto operator=(node const&) noexcept -> node & = delete;
				inline auto operator=(node&&) noexcept -> node & = delete;
				inline ~node(void) noexcept = delete;
				node* _next;
				size_type _index;
				group* _value;
			};
		public:
			inline explicit group_array(void) noexcept = default;
			inline explicit group_array(group_array const&) noexcept = delete;
			inline explicit group_array(group_array&&) noexcept = delete;
			inline auto operator=(group_array const&) noexcept -> group_array & = delete;
			inline auto operator=(group_array&&) noexcept -> group_array & = delete;
			inline ~group_array(void) noexcept = default;

			inline void initialize(size_type const capacity) noexcept {
				_size = 0;
				_capacity = capacity;
				_array = reinterpret_cast<node*>(malloc(sizeof(node) * capacity));
				node* current = _array;
				node* next = current + 1;
				for (size_type index = 0; index < capacity - 1; ++index, current = next++) {
					current->_next = next;
					current->_index = index;
					current->_value = nullptr;
				}
#pragma warning(suppress: 6011)
				current->_next = nullptr;
				current->_index = capacity - 1;
				current->_value = nullptr;
				_head = reinterpret_cast<unsigned long long>(_array);
			}
			inline void finalize(void) noexcept {
				free(_array);
			}
			template<typename type, typename... argument>
			inline auto acquire(server& server_, argument&&... arg) noexcept -> group* {
				for (;;) {
					unsigned long long head = _head;
					node* current = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
					if (nullptr == current)
						return nullptr;
					unsigned long long next = reinterpret_cast<unsigned long long>(current->_next) + (0xFFFF800000000000ULL & head) + 0x0000800000000000ULL;
					if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head)) {
						_InterlockedIncrement(&_size);

						auto& memory_pool = data_structure::_thread_local::memory_pool<type, 128, false>::instance();
						group* group_ = &memory_pool.allocate(std::forward<argument>(arg)...);
						group_->_key |= current->_index;
						group_->_server = &server_;
						group_->_destructor = group::destructor<type>;
						current->_value = group_;
						return current->_value;
					}
				}
			}
			inline void release(group& value) noexcept {
				value._destructor(&value);
				node* current = _array + (value._key & 0xffff);
				for (;;) {
					unsigned long long head = _head;
					current->_next = reinterpret_cast<node*>(head & 0x00007FFFFFFFFFFFULL);
					unsigned long long next = reinterpret_cast<unsigned long long>(current) + (head & 0xFFFF800000000000ULL);
					if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head))
						break;
				}
				_InterlockedDecrement(&_size);
			}
			inline auto operator[](unsigned long long const key) noexcept -> group& {
				return *_array[key & 0xffff]._value;
			}

			unsigned long long _head;
			node* _array;
			size_type _size;
			size_type _capacity;
		};
		class task_queue final : protected data_structure::lockfree::queue<task*> {
		private:
			using base = data_structure::lockfree::queue<task*>;
		public:
			inline explicit task_queue(void) noexcept = default;
			inline explicit task_queue(task_queue const&) noexcept = delete;
			inline explicit task_queue(task_queue&&) noexcept = delete;
			inline auto operator=(task_queue const&) noexcept -> task_queue & = delete;
			inline auto operator=(task_queue&&) noexcept -> task_queue & = delete;
			inline ~task_queue(void) noexcept = default;

			inline void push(task& task_) noexcept {
				base::emplace(&task_);
				_InterlockedIncrement(&_size);
				system_component::wait_on_address::wake_single(&_size);
			}
			inline auto pop(void) noexcept -> task& {
				unsigned long long head = _head;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				unsigned long long next = address->_next;

				if (0x10000 > (0x00007FFFFFFFFFFFULL & next))
					__debugbreak();
				unsigned long long tail = _tail;
				if (tail == head)
					_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);

				task& result = *reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & next)->_value;
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
			inline void wake(void) noexcept {
				system_component::wait_on_address::wake_single(&_size);
			}
			inline bool wait(void* compare, unsigned long wait_time) noexcept {
				return system_component::wait_on_address::wait(&_size, compare, sizeof(size_type), wait_time);
			}
		private:
			size_type _size = 0;
		};
		inline static auto less(task* const& source, task* const& destination) noexcept -> std::strong_ordering {
			return source->_time <=> destination->_time;
		}
		using ready_queue = data_structure::priority_queue<task*, less>;

#pragma warning(suppress: 26495)
		inline explicit scheduler(void) noexcept = default;
		inline explicit scheduler(scheduler const&) noexcept = delete;
		inline explicit scheduler(scheduler&&) noexcept = delete;
		inline auto operator=(scheduler const&) noexcept -> scheduler & = delete;
		inline auto operator=(scheduler&&) noexcept -> scheduler & = delete;
		inline ~scheduler(void) noexcept = default;

		inline void initialize(void) noexcept {
			_active = 0;
			_size = 0;
		}
		inline void finalize(void) noexcept {
			_InterlockedExchange(&_active, -1);
			_task_queue.wake();
			_thread.wait_for_single(INFINITE);
			_thread.close();
		}
		inline void push(task& task_) noexcept {
			_task_queue.push(task_);
		}
		inline bool wait(unsigned long wait_time) noexcept {
			return _task_queue.wait(&_active, wait_time);
		}

		size_type _active;
		system_component::thread _thread;
		task_queue _task_queue;
		size_type _size;
	};
protected:
	using message_pointer = session::message_pointer;
	using view_pointer = session::view_pointer;
	using group = scheduler::group;

#pragma warning(suppress: 26495)
	inline explicit server(void) noexcept {
		utility::crash_dump();
		system_component::time::multimedia::begin_period(1);
		system_component::wsa_start_up();
		database::mysql::initialize();
		//utility::logger::instance().create("server", L"server.log");

		SYSTEM_INFO info;
		GetSystemInfo(&info);
		_number_of_processor = info.dwNumberOfProcessors;

		auto& command_ = command::instance();
		command_.add("log_output", [&](command::parameter* param) noexcept -> int {
			unsigned char output = 0;
			for (size_type index = 1; index < param->size(); ++index) {
				if ("file" == param->get_string(index))
					output |= utility::logger::output::file;
				else if ("console" == param->get_string(index))
					output |= utility::logger::output::console;
			}
			utility::logger::instance().set_output(output);
			return 0;
			});
		command_.add("log_level", [&](command::parameter* param) noexcept -> int {
			if ("trace" == param->get_string(1))
				utility::logger::instance().set_level(utility::logger::level::trace);
			else if ("debug" == param->get_string(1))
				utility::logger::instance().set_level(utility::logger::level::debug);
			else if ("info" == param->get_string(1))
				utility::logger::instance().set_level(utility::logger::level::info);
			else if ("warning" == param->get_string(1))
				utility::logger::instance().set_level(utility::logger::level::warning);
			else if ("error" == param->get_string(1))
				utility::logger::instance().set_level(utility::logger::level::error);
			else if ("fatal" == param->get_string(1))
				utility::logger::instance().set_level(utility::logger::level::fatal);
			return 0;
			});
		command_.add("iocp_concurrent", [&](command::parameter* param) noexcept -> int {
			_concurrent_thread_count = param->get_int(1);
			return 0;
			});
		command_.add("iocp_worker", [&](command::parameter* param) noexcept -> int {
			_worker_thread_count = param->get_int(1);
			return 0;
			});
		command_.add("group_max", [&](command::parameter* param) noexcept -> int {
			_group_array_max = param->get_int(1);
			return 0;
			});
		command_.add("session_max", [&](command::parameter* param) noexcept -> int {
			_session_array_max = param->get_int(1);
			return 0;
			});
		command_.add("send_mode", [&](command::parameter* param) noexcept -> int {
			auto& send_mode = param->get_string(1);
			if ("fast" == send_mode)
				_send_mode = false;
			else if ("direct" == send_mode)
				_send_mode = true;
			return 0;
			});
		command_.add("send_frame", [&](command::parameter* param) noexcept -> int {
			_send_frame = param->get_int(1);
			return 0;
			});
		command_.add("timeout_duration", [&](command::parameter* param) noexcept -> int {
			_timeout_duration = param->get_int(1);
			return 0;
			});
		command_.add("timeout_frame", [&](command::parameter* param) noexcept -> int {
			_timeout_frame = param->get_int(1);
			return 0;
			});
		command_.add("tcp_ip", [&](command::parameter* param) noexcept -> int {
			_listen_socket_ip = param->get_string(1);
			return 0;
			});
		command_.add("tcp_port", [&](command::parameter* param) noexcept -> int {
			_listen_socket_port = param->get_int(1);
			return 0;
			});
		command_.add("tcp_backlog", [&](command::parameter* param) noexcept -> int {
			_listen_socket_backlog = param->get_int(1);
			return 0;
			});
		command_.add("header_code", [&](command::parameter* param) noexcept -> int {
			_header_code = param->get_int(1);
			return 0;
			});
		command_.add("header_key", [&](command::parameter* param) noexcept -> int {
			_header_fixed_key = param->get_int(1);
			return 0;
			});
		command_.add("server_start", [&](command::parameter* param) noexcept -> int {
			this->start();
			return 0;
			});
		command_.add("server_stop", [&](command::parameter* param) noexcept -> int {
			this->stop();
			return 0;
			});
	};
	inline explicit server(server const&) noexcept = delete;
	inline explicit server(server&&) noexcept = delete;
	inline auto operator=(server const&) noexcept -> server & = delete;
	inline auto operator=(server&&) noexcept -> server & = delete;
	inline ~server(void) noexcept {
		database::mysql::end();
		system_component::time::multimedia::end_period(1);
		system_component::wsa_clean_up();
	};

	inline void start(void) noexcept {
		_complation_port.create(_concurrent_thread_count);
		for (size_type index = 0; index < _worker_thread_count; ++index)
			_worker_thread.emplace_back(&server::worker, 0, this);
		_scheduler.initialize();
		_scheduler._thread.begin(&server::schedule, 0, this);

		_group_array.initialize(_group_array_max);
		_session_array.initialize(_session_array_max);
		if (0 != _send_frame)
			do_create_function(&server::send, this);
		if (0 != _timeout_duration)
			do_create_function(&server::timeout, this);
		auto& query = utility::performance_data_helper::query::instance();
		_processor_total_time = query.add_counter(L"\\Processor(_Total)\\% Processor Time");
		_processor_user_time = query.add_counter(L"\\Processor(_Total)\\% User Time");
		_processor_kernel_time = query.add_counter(L"\\Processor(_Total)\\% Privileged Time");
		_process_total_time = query.add_counter(L"\\Process(monitor-server)\\% Processor Time");
		_process_user_time = query.add_counter(L"\\Process(monitor-server)\\% User Time");
		_process_kernel_time = query.add_counter(L"\\Process(monitor-server)\\% Privileged Time");
		_memory_available_byte = query.add_counter(L"\\Memory\\Available Bytes");
		_memory_pool_nonpaged_byte = query.add_counter(L"\\Memory\\Pool Nonpaged Bytes");
		_process_private_byte = query.add_counter(L"\\Process(monitor-server)\\Private Bytes");
		_process_pool_nonpaged_byte = query.add_counter(L"\\Process(monitor-server)\\Pool Nonpaged Bytes");
		_tcpv4_segments_received_sec = query.add_counter(L"\\TCPv4\\Segments Received/sec");
		_tcpv4_segments_sent_sec = query.add_counter(L"\\TCPv4\\Segments Sent/sec");
		_tcpv4_segments_retransmitted_sec = query.add_counter(L"\\TCPv4\\Segments Retransmitted/sec");
		do_create_function(&server::monit, this);

		on_start();

		system_component::socket_address_ipv4 socket_address;
		socket_address.set_address(_listen_socket_ip.c_str());
		socket_address.set_port(_listen_socket_port);
		_listen_socket.create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		_listen_socket.set_linger(1, 0);
		if (true == _send_mode)
			_listen_socket.set_send_buffer(0);
		_listen_socket.bind(socket_address);
		_listen_socket.listen(_listen_socket_backlog);
		_accept_thread.begin(&server::accept, 0, this);
	}
	inline void stop(void) noexcept {
		_listen_socket.close();
		_accept_thread.wait_for_single(INFINITE);
		_accept_thread.close();
		for (auto& iter : _session_array) {
			if (iter._value.acquire())
				iter._value.cancel();
			if (iter._value.release()) {
				on_destroy_session(iter._value._key);
				_session_array.release(iter._value);
			}
		}
		_session_array.wait();

		on_stop();

		_scheduler.finalize();
		_session_array.finalize();
		_group_array.finalize();


		for (size_type index = 0; index < _worker_thread.size(); ++index)
			_complation_port.post_queue_state(0, 0, nullptr);
		HANDLE handle[128];
		for (unsigned int index = 0; index < _worker_thread.size(); ++index)
			handle[index] = _worker_thread[index].data();
		system_component::handle::wait_for_multiple(_worker_thread.size(), handle, true, INFINITE);
		_worker_thread.clear();
		_complation_port.close();
	}
private:
	inline void accept(void) noexcept {
		for (;;) {
			auto [socket, socket_address] = _listen_socket.accept();
			if (INVALID_SOCKET == socket.data())
				break;
			++_accept_total_count;
			++_accept_tps;
			if (false == on_accept_socket(socket_address))
				socket.close();
			else {
				session* session_ = _session_array.acquire();
				if (nullptr == session_)
					socket.close();
				else {
					session_->initialize(std::move(socket), _timeout_duration);
					_complation_port.connect(session_->_socket, reinterpret_cast<ULONG_PTR>(session_));
					on_create_session(session_->_key);
					session_->receive();
					if (session_->release()) {
						on_destroy_session(session_->_key);
						_session_array.release(*session_);
					}
				}
			}
		}
	}
	inline void worker(void) noexcept {
		on_worker_start();
		for (;;) {
			auto [result, transferred, key, overlapped] = _complation_port.get_queue_state(INFINITE);
			switch (static_cast<post_queue_state>(key)) {
				using enum post_queue_state;
			case close_worker:
				return;
			case destory_session: {
				session& session_ = *reinterpret_cast<session*>(overlapped);
				on_destroy_session(session_._key);
				_session_array.release(session_);
			} break;
			case destory_group: {
				scheduler::group& group_ = *reinterpret_cast<scheduler::group*>(overlapped);
				on_destory_group(group_._key);
				_group_array.release(group_);
			} break;
			case excute_task: {
				scheduler::task& task = *reinterpret_cast<scheduler::task*>(overlapped);
				if (task.excute()) {
					_scheduler.push(task);
					continue;
				}
				switch (task._type) {
				case scheduler::task::type::function: {
					scheduler::function& function_ = *static_cast<scheduler::function*>(&task);
					_InterlockedDecrement(&_scheduler._size);
					auto& memory_pool = data_structure::_thread_local::memory_pool<scheduler::function>::instance();
					memory_pool.deallocate(function_);
				} break;
				case scheduler::task::type::group: {
					scheduler::group& group_ = *static_cast<scheduler::group*>(&task);
					_InterlockedDecrement(&_scheduler._size);
					if (group_.release()) {
						on_destory_group(group_._key);
						_group_array.release(group_);
					}
				} break;
				default:
					__debugbreak();
				}
			} break;
			default: {
				session& session_ = *reinterpret_cast<session*>(key);
				if (0 != transferred) {
					if (&session_._recv_overlapped.data() == overlapped) {
						session_._receive_message->move_rear(transferred);
						session_._timeout_currnet = GetTickCount64();

						for (;;) {
							if (sizeof(session::header) > session_._receive_message->size())
								break;
							session::header header_;
							session_._receive_message->peek(reinterpret_cast<unsigned char*>(&header_), sizeof(session::header));
							if (header_._code != _header_code) {
								session_.cancel();
								break;
							}
							if (header_._length > 256) { //юс╫ц
								session_.cancel();
								break;
							}
							if (sizeof(session::header) + header_._length > session_._receive_message->size())
								break;
							session_._receive_message->pop(sizeof(session::header));

							auto& memory_pool = data_structure::_thread_local::memory_pool<session::view>::instance();
							session::view_pointer view_ptr(&memory_pool.allocate(session_._receive_message, session_._receive_message->front() - 1, session_._receive_message->front() + header_._length));
							session_._receive_message->pop(header_._length);

							//----------------------------------------------------------
							unsigned char p = 0;
							unsigned char e = 0;
							unsigned char temp = 0;
							size_type index = 0;
							unsigned char check_sum = 0;

							for (auto iter = view_ptr->begin(), end = view_ptr->end(); iter != end; ++iter) {
								temp = (*iter) ^ (e + _header_fixed_key + index + 1);
								e = (*iter);
								(*iter) = temp ^ (p + header_._random_key + index + 1);
								p = temp;
								check_sum += (*iter);
								index++;
							}

							unsigned char check_sum_;
							(*view_ptr) >> check_sum_;
							check_sum -= check_sum_;
							if (check_sum != check_sum_) {
								session_.cancel();
								break;
							}
							//----------------------------------------------------------

							session_._receive_queue.push(view_ptr);
							_InterlockedIncrement(&_receive_tps);
						}

						if (session_.acquire_receive()) {
							while (0 == (session_._receive_count & 0x40000000) && !session_._receive_queue.empty()) {
								auto view_ptr = session_._receive_queue.pop();
								if (false == on_receive_session(session_._key, view_ptr)) {
									session_.cancel();
									break;
								}
							}
						}
						if (session_.release_receive()) {
							auto& group_ = _group_array[session_._group_key];
							if (group_.acquire(session_._group_key))
								group_.do_enter_session(session_._key);
							else
								_InterlockedAnd((long*)&session_._receive_count, 0x3FFFFFFF);
							if (group_.release())
								_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_group), reinterpret_cast<OVERLAPPED*>(&group_));
						}

						if (session_.ready_receive())
							session_.receive();
					}
					else {
						_interlockedadd((volatile long*)&_send_tps, session_._send_size);
						session_.finish_send();
						if (0 == _send_frame)
							session_.send();
					}
				}
				else
					session_._cancel_flag = 1;
				if (session_.release()) {
					on_destroy_session(session_._key);
					_session_array.release(session_);
				}
			} break;
			}
		}
	}
	inline void schedule(void) {
		scheduler::ready_queue _ready_queue;
		unsigned long wait_time = INFINITE;
		while (0 == _scheduler._active || 0 != _scheduler._size) {
			bool result = _scheduler.wait(wait_time);
			if (result) {
				while (!_scheduler._task_queue.empty())
					_ready_queue.push(&_scheduler._task_queue.pop());
			}
			wait_time = INFINITE;
			unsigned long time = system_component::time::multimedia::get_time();
			while (!_ready_queue.empty()) {
				auto task_ = _ready_queue.top();
				if (time < task_->_time) {
					wait_time = static_cast<unsigned long>(task_->_time - time);
					break;
				}
				_ready_queue.pop();
				_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::excute_task), reinterpret_cast<OVERLAPPED*>(task_));
			}
		}
	}
	inline int send(void) noexcept {
		for (auto iter = _session_array.begin(), end = _session_array.end(); iter != end; ++iter) {
			if (iter->_value.acquire()) {
				iter->_value.send();
			}
			if (iter->_value.release()) {
				on_destroy_session(iter->_value._key);
				_session_array.release(iter->_value);
			}
		}
		if (-1 == _scheduler._active)
			return -1;
		return _send_frame;
	}
	inline int timeout(void) noexcept {
		for (auto iter = _session_array.begin(), end = _session_array.end(); iter != end; ++iter) {
			if (iter->_value.acquire()) {
				if (iter->_value._timeout_currnet + iter->_value._timeout_duration < GetTickCount64()) {
					iter->_value.cancel();
					++_timeout_total_count;
				}
			}
			if (iter->_value.release()) {
				on_destroy_session(iter->_value._key);
				_session_array.release(iter->_value);
			}
		}
		if (-1 == _scheduler._active)
			return -1;
		return _timeout_frame;
	}
	inline int monit(void) noexcept {
		auto& query = utility::performance_data_helper::query::instance();
		query.collect_query_data();

		printf("--------------------------------------\n"\
			"[ System Monitor ]\n"\
			"CPU Usage\n"\
			" System  - Total  :   %f %%\n"\
			"           User   :   %f %%\n"\
			"           Kernel :   %f %%\n"\
			" Process - Total  :   %f %%\n"\
			"           User   :   %f %%\n"\
			"           Kernel :   %f %%\n"\
			"Memory Usage\n"\
			" System  - Available :   %f GB\n"\
			"           Non-Paged :   %f MB\n"\
			" Process - Private   :   %f MB\n"\
			"           Non-Paged :   %f MB\n"\
			"Network Usage\n"\
			" Receive        :   %f\n"\
			" Send           :   %f\n"\
			" Retransmission :   %f\n",
			_processor_total_time.get_formatted_value(PDH_FMT_DOUBLE).doubleValue,
			_processor_user_time.get_formatted_value(PDH_FMT_DOUBLE).doubleValue,
			_processor_kernel_time.get_formatted_value(PDH_FMT_DOUBLE).doubleValue,
			_process_total_time.get_formatted_value(PDH_FMT_DOUBLE | PDH_FMT_NOCAP100).doubleValue / _number_of_processor,
			_process_user_time.get_formatted_value(PDH_FMT_DOUBLE | PDH_FMT_NOCAP100).doubleValue / _number_of_processor,
			_process_kernel_time.get_formatted_value(PDH_FMT_DOUBLE | PDH_FMT_NOCAP100).doubleValue / _number_of_processor,
			_memory_available_byte.get_formatted_value(PDH_FMT_DOUBLE).doubleValue / 0x40000000,
			_memory_pool_nonpaged_byte.get_formatted_value(PDH_FMT_DOUBLE).doubleValue / 0x100000,
			_process_private_byte.get_formatted_value(PDH_FMT_DOUBLE).doubleValue / 0x100000,
			_process_pool_nonpaged_byte.get_formatted_value(PDH_FMT_DOUBLE).doubleValue / 0x100000,
			_tcpv4_segments_received_sec.get_formatted_value(PDH_FMT_DOUBLE).doubleValue,
			_tcpv4_segments_sent_sec.get_formatted_value(PDH_FMT_DOUBLE).doubleValue,
			_tcpv4_segments_retransmitted_sec.get_formatted_value(PDH_FMT_DOUBLE).doubleValue);

		auto& message_pool = data_structure::_thread_local::memory_pool<session::message>::instance();
		auto& view_pool = data_structure::_thread_local::memory_pool<session::view>::instance();
		printf("--------------------------------------\n"\
			"[ Server Monitor ]\n"\
			"Connect\n"\
			" Accept Total  :   %llu\n"\
			" Timeout Total :   %llu\n"\
			" Session Count :   %u\n"\
			" Group Count   :   %u\n"\
			"Traffic\n"\
			" Accept  :   %u TPS\n"\
			" Receive :   %u TPS\n"\
			" Send    :   %u TPS\n"\
			"Resource Usage\n"\
			" Message  - Pool Count :   %u\n"\
			"            Use Count  :   %u\n"\
			" View     - Pool Count :   %u\n"\
			"            Use Count  :   %u\n",
			_accept_total_count, _timeout_total_count, _session_array._size, _group_array._size, _accept_tps, _receive_tps, _send_tps,
			message_pool._stack._capacity, message_pool._use_count, view_pool._stack._capacity, view_pool._use_count);

		on_monit();

		_accept_tps = 0;
		_receive_tps = 0;
		_send_tps = 0;

		if (-1 == _scheduler._active)
			return -1;
		return 1000;
	}
protected:
	inline virtual void on_start(void) noexcept = 0;
	inline virtual void on_worker_start(void) noexcept = 0;
	inline virtual void on_stop(void) noexcept = 0;
	inline virtual void on_monit(void) noexcept = 0;

	inline virtual bool on_accept_socket(system_component::socket_address_ipv4& socket_address) noexcept = 0;
	inline virtual void on_create_session(unsigned long long key) noexcept = 0;
	inline virtual bool on_receive_session(unsigned long long key, session::view_pointer& view_ptr) noexcept = 0;
	inline virtual void on_destroy_session(unsigned long long key) noexcept = 0;
	inline auto do_create_session(char const* const address, unsigned short port) noexcept -> unsigned long long {
		system_component::socket_address_ipv4 socket_address;
		socket_address.set_address(address);
		socket_address.set_port(port);
		system_component::socket socket(AF_INET, SOCK_STREAM, 0);
		socket.set_linger(1, 0);
		socket.connect(socket_address);

		session& session_ = *_session_array.acquire();
		session_.initialize(std::move(socket), _timeout_duration);
		_complation_port.connect(session_._socket, reinterpret_cast<ULONG_PTR>(&session_));
		session_.receive();
		if (session_.release()) {
			on_destroy_session(session_._key);
			_session_array.release(session_);
			return 0;
		}
		return session_._key;
	}
	inline void do_send_session(unsigned long long key, session::view_pointer& view_ptr) noexcept {
		session& session_ = _session_array[key];
		if (session_.acquire(key)) {
			if (512 > session_._send_queue.size()) {
				session::header* header_ = reinterpret_cast<session::header*>(view_ptr->data()->data() + view_ptr->front());
				if (0 == header_->_code) {
					header_->_code = _header_code;
					header_->_length = view_ptr->size() - 5;
					auto random_key = header_->_random_key = rand() % 256;
					auto header_fixed_key = _header_fixed_key;
					unsigned char check_sum = 0;

					auto end = view_ptr->end();
					for (auto iter = view_ptr->begin() + sizeof(session::header); iter != end; ++iter) {
						check_sum += *iter;
					}
					header_->_check_sum = check_sum;

					unsigned char p = 0;
					unsigned char e = 0;
					size_type index = 0;
					for (auto iter = view_ptr->begin() + sizeof(session::header) - 1; iter != end; ++iter) {
						p = *iter ^ (p + random_key + index + 1);
						e = p ^ (e + header_fixed_key + index + 1);
						*iter = e;
						index++;
					}
				}
				session_._send_queue.push(view_ptr);
				if (0 == _send_frame)
					session_.send();
			}
			else {
				session_.cancel();
			}
		}
		if (session_.release())
			_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_session), reinterpret_cast<OVERLAPPED*>(&session_));
	}
	inline void do_destroy_session(unsigned long long key) noexcept {
		session& session_ = _session_array[key];
		if (session_.acquire(key))
			session_.cancel();
		if (session_.release())
			_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_session), reinterpret_cast<OVERLAPPED*>(&session_));
	}
	inline void do_set_timeout_session(unsigned long long key, unsigned long long duration) noexcept {
		session& session_ = _session_array[key];
		if (session_.acquire(key))
			session_._timeout_duration = duration;
		if (session_.release())
			_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_session), reinterpret_cast<OVERLAPPED*>(&session_));
	}
	inline auto do_get_session_ip(unsigned long long key) noexcept -> std::optional<std::wstring> {
		session& session_ = _session_array[key];
		std::optional<std::wstring> result = std::nullopt;
		if (session_.acquire(key)) {
			auto address = session_._socket.get_remote_socket_address();
			if (address)
				result = address->get_address();
		}
		if (session_.release())
			_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_session), reinterpret_cast<OVERLAPPED*>(&session_));
		return result;
	}

	inline virtual void on_destory_group(unsigned long long key) noexcept = 0;
	template<typename type, typename... argument>
	inline auto do_create_group(argument&&... arg) noexcept -> unsigned long long {
		if (0 == _scheduler._active) {
			_InterlockedIncrement(&_scheduler._size);
			if (0 == _scheduler._active) {
				group& group_ = *_group_array.acquire<type>(*this, std::forward<argument>(arg)...);
				_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::excute_task), reinterpret_cast<OVERLAPPED*>(static_cast<scheduler::task*>(&group_)));
				return group_._key;
			}
			else
				_InterlockedDecrement(&_scheduler._size);
		}
		return 0;
	}
	inline void do_destroy_group(unsigned long long key) noexcept {
		auto& group_ = _group_array[key];
		if (group_.acquire(key))
			group_.cancel();
		if (group_.release())
			_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_group), reinterpret_cast<OVERLAPPED*>(&group_));
	}
	inline void do_enter_session_to_group(unsigned long long session_key, unsigned long long group_key) noexcept {
		session& session_ = _session_array[session_key];
		if (session_.acquire(session_key)) {
			if (session_.acquire_receive())
				if (session_.group())
					session_._group_key = group_key;
			if (session_.release_receive()) {
				auto& group_ = _group_array[session_._group_key];
				if (group_.acquire(session_._group_key))
					group_.do_enter_session(session_key);
				else
					_InterlockedAnd((long*)&session_._receive_count, 0x3FFFFFFF);
				if (group_.release())
					_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_group), reinterpret_cast<OVERLAPPED*>(&group_));
			}
		}
		if (session_.release())
			_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::destory_session), reinterpret_cast<OVERLAPPED*>(&session_));
	}

	template <typename function, typename... argument>
	inline void do_create_function(function&& func, argument&&... arg) noexcept {
		if (0 == _scheduler._active) {
			_InterlockedIncrement(&_scheduler._size);
			if (0 == _scheduler._active) {
				auto& memory_pool = data_structure::_thread_local::memory_pool<scheduler::function>::instance();
				scheduler::task* task_(&memory_pool.allocate(std::forward<function>(func), std::forward<argument>(arg)...));
				_complation_port.post_queue_state(0, static_cast<uintptr_t>(post_queue_state::excute_task), reinterpret_cast<OVERLAPPED*>(task_));
			}
			else
				_InterlockedDecrement(&_scheduler._size);
		}
	}

	inline static auto create_message(size_type const size) noexcept -> session::view_pointer {
		auto& message_pool = data_structure::_thread_local::memory_pool<session::message>::instance();
		thread_local session::message_pointer message_ptr;
		if (nullptr == message_ptr || message_ptr->capacity() - message_ptr->rear() < sizeof(session::header) + size) {
			session::message_pointer new_message_ptr(&message_pool.allocate());
			new_message_ptr->clear();
			message_ptr = new_message_ptr;
		}
		auto& view_pool = data_structure::_thread_local::memory_pool<session::view>::instance();
		session::view_pointer view_ptr(&view_pool.allocate(message_ptr, message_ptr->rear(), message_ptr->rear()));
		message_ptr->move_rear(sizeof(session::header) + size);

		session::header header_{};
		view_ptr->push(reinterpret_cast<unsigned char*>(&header_), sizeof(session::header));
		return view_ptr;
	}
private:
	system_component::inputoutput_completion_port _complation_port;
	data_structure::vector<system_component::thread> _worker_thread;
	scheduler _scheduler;
	scheduler::group_array _group_array;
	session_array _session_array;
	system_component::socket _listen_socket;
	system_component::thread _accept_thread;
public:
	size_type _concurrent_thread_count;
	size_type _worker_thread_count;
	size_type _group_array_max;
	size_type _session_array_max;
	bool _send_mode;
	size_type _send_frame;
	unsigned long long _timeout_duration;
	size_type _timeout_frame;
	std::string _listen_socket_ip;
	size_type _listen_socket_port;
	size_type _listen_socket_backlog;
	unsigned char _header_code;
	unsigned char _header_fixed_key;

	utility::performance_data_helper::query::counter _processor_total_time;
	utility::performance_data_helper::query::counter _processor_user_time;
	utility::performance_data_helper::query::counter _processor_kernel_time;
	utility::performance_data_helper::query::counter _process_total_time;
	utility::performance_data_helper::query::counter _process_user_time;
	utility::performance_data_helper::query::counter _process_kernel_time;
	utility::performance_data_helper::query::counter _memory_available_byte;
	utility::performance_data_helper::query::counter _memory_pool_nonpaged_byte;
	utility::performance_data_helper::query::counter _process_private_byte;
	utility::performance_data_helper::query::counter _process_pool_nonpaged_byte;
	utility::performance_data_helper::query::counter _tcpv4_segments_received_sec;
	utility::performance_data_helper::query::counter _tcpv4_segments_sent_sec;
	utility::performance_data_helper::query::counter _tcpv4_segments_retransmitted_sec;
	unsigned long long _accept_total_count = 0;
	unsigned long long _timeout_total_count = 0;
	size_type _accept_tps = 0;
	size_type _receive_tps = 0;
	size_type _send_tps = 0;

	unsigned long _number_of_processor;
};