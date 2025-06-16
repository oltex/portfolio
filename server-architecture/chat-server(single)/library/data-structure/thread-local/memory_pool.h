#pragma once
#include "../../design-pattern/thread-local/singleton.h"
#include "../lockfree/memory_pool.h"
#include "../pair.h"

namespace data_structure::_thread_local {
	template<typename type, size_t bucket_size = 128, bool use_union = true>
	class memory_pool final : public design_pattern::_thread_local::singleton<memory_pool<type, bucket_size, use_union>> {
	private:
		friend class design_pattern::_thread_local::singleton<memory_pool<type, bucket_size, use_union>>;
		using size_type = unsigned int;
		union union_node final {
			inline explicit union_node(void) noexcept = delete;
			inline explicit union_node(union_node const&) noexcept = delete;
			inline explicit union_node(union_node&&) noexcept = delete;
			inline auto operator=(union_node const&) noexcept = delete;
			inline auto operator=(union_node&&) noexcept = delete;
			inline ~union_node(void) noexcept = delete;
			union_node* _next;
			type _value;
		};
		struct strcut_node {
			inline explicit strcut_node(void) noexcept = delete;
			inline explicit strcut_node(strcut_node const&) noexcept = delete;
			inline explicit strcut_node(strcut_node&&) noexcept = delete;
			inline auto operator=(strcut_node const&) noexcept = delete;
			inline auto operator=(strcut_node&&) noexcept = delete;
			inline ~strcut_node(void) noexcept = delete;
			strcut_node* _next;
			type _value;
		};
		using node = typename std::conditional<use_union, union union_node, struct strcut_node>::type;
		class stack final {
		private:
			struct bucket final {
				inline explicit bucket(void) noexcept = delete;
				inline explicit bucket(bucket const&) noexcept = delete;
				inline explicit bucket(bucket&&) noexcept = delete;
				inline auto operator=(bucket const&) noexcept = delete;
				inline auto operator=(bucket&&) noexcept = delete;
				inline ~bucket(void) noexcept = delete;
				bucket* _next;
				node* _value;
				size_type _size;
			};
			inline static consteval size_type power_of_two(size_type number, size_type square = 1) noexcept {
				return square >= number ? square : power_of_two(number, square << 1);
			}
			static constexpr size_type _align = power_of_two(sizeof(node) * bucket_size);
		public:
			inline explicit stack(void) noexcept
				: _head(0) {
			}
			inline explicit stack(stack const& rhs) noexcept = delete;
			inline explicit stack(stack&& rhs) noexcept = delete;
			inline auto operator=(stack const& rhs) noexcept -> stack & = delete;
			inline auto operator=(stack&& rhs) noexcept -> stack & = delete;
			inline ~stack(void) noexcept {
				node** _node_array = reinterpret_cast<node**>(malloc(sizeof(node*) * _capacity));
				size_type _node_array_index = 0;

				bucket* head = reinterpret_cast<bucket*>(0x00007FFFFFFFFFFFULL & _head);
				while (nullptr != head) {
					bucket* next = head->_next;

					node* _node = head->_value;
					for (size_type index = 0; index < head->_size; ++index) {
						if (0 == (reinterpret_cast<uintptr_t>(_node) & (_align - 1))) {
#pragma warning(suppress: 6011)
							_node_array[_node_array_index] = _node;
							_node_array_index++;
						}
						_node = _node->_next;
					}
					_memory_pool.deallocate(*head);
					head = next;
				}

				for (size_type index = 0; index < _node_array_index; ++index)
					_aligned_free(_node_array[index]);
				free(_node_array);
			};
		public:
			inline void push(node* value, size_type size) noexcept {
				bucket* current = &_memory_pool.allocate();
				current->_value = value;
				current->_size = size;

				for (;;) {
					unsigned long long head = _head;
					current->_next = reinterpret_cast<bucket*>(0x00007FFFFFFFFFFFULL & head);
					unsigned long long next = reinterpret_cast<unsigned long long>(current) + (0xFFFF800000000000ULL & head);
					if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head))
						break;
				}
			}
			inline auto pop(void) noexcept -> pair<node*, size_type> {
				for (;;) {
					unsigned long long head = _head;
					bucket* address = reinterpret_cast<bucket*>(0x00007FFFFFFFFFFFULL & head);
					if (nullptr == address) {
						pair<node*, size_type> result{ reinterpret_cast<node*>(_aligned_malloc(sizeof(node) * bucket_size, _align)), bucket_size };
						_InterlockedIncrement(&_capacity);

						node* current = result._first;
						node* next = current + 1;
						for (size_type index = 0; index < bucket_size - 1; ++index, current = next++)
#pragma warning(suppress: 6011)
							current->_next = next;
						current->_next = nullptr;

						return result;
					}
					unsigned long long next = reinterpret_cast<unsigned long long>(address->_next) + (0xFFFF800000000000ULL & head) + 0x0000800000000000ULL;
					if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head)) {
						pair<node*, size_type> result{ address->_value, address->_size };
						_memory_pool.deallocate(*address);
						return result;
					}
				}
			}
		private:
			unsigned long long _head;
			lockfree::memory_pool<bucket> _memory_pool;
		public:
			size_type _capacity = 0;
		};
	private:
		inline explicit memory_pool(void) noexcept = default;
		inline explicit memory_pool(memory_pool const& rhs) noexcept = delete;
		inline explicit memory_pool(memory_pool&& rhs) noexcept = delete;
		inline auto operator=(memory_pool const& rhs) noexcept -> memory_pool & = delete;
		inline auto operator=(memory_pool&& rhs) noexcept -> memory_pool & = delete;
		inline ~memory_pool(void) noexcept {
			if (_size > bucket_size) {
				_stack.push(_head, _size - bucket_size);
				_head = _break;
				_size = bucket_size;
			}
			_stack.push(_head, _size);
		};
	public:
		template<typename... argument>
		inline auto allocate(argument&&... arg) noexcept -> type& {
			node* current;
			if (0 == _size) {
				auto [value, size] = _stack.pop();
				_head = value;
				_size = size;
			}
			current = _head;
			_head = current->_next;
			if constexpr (std::is_class_v<type>) {
				if constexpr (std::is_constructible_v<type, argument...>)
					::new(reinterpret_cast<void*>(&current->_value)) type(std::forward<argument>(arg)...);
			}
			else if constexpr (1 == sizeof...(arg))
				current->_value = type(std::forward<argument>(arg)...);
			--_size;
			_InterlockedIncrement(&_use_count);
			return current->_value;
		}
		inline void deallocate(type& value) noexcept {
			if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
				value.~type();
			if constexpr (true == use_union) {
				reinterpret_cast<node*>(&value)->_next = _head;
				_head = reinterpret_cast<node*>(&value);
			}
			else {
				reinterpret_cast<node*>(reinterpret_cast<uintptr_t*>(&value) - 1)->_next = _head;
				_head = reinterpret_cast<node*>(reinterpret_cast<uintptr_t*>(&value) - 1);
			}
			++_size;
			_InterlockedDecrement(&_use_count);

			if (bucket_size == _size)
				_break = _head;
			else if (bucket_size * 2 == _size) {
				_stack.push(_head, bucket_size);
				_head = _break;
				_size -= bucket_size;
			}
		}
	private:
		node* _head = nullptr;
		node* _break = nullptr;
		size_type _size = 0;
	public:
		inline static stack _stack;
		inline static size_type _use_count = 0;
	};
}