#pragma once
#include "library/memory.h"
#include "library/tls_singleton.h"
#include "library/lockfree_pool.h"
#include "library/pair.h"

namespace library::_thread_local {
	template<typename type, size_t bucket_size = 1024, bool placement = true, bool compress = true>
	class pool final : public singleton<pool<type, bucket_size, compress>> {
	private:
		friend class singleton<pool<type, bucket_size, compress>>;
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
		struct strcut_node final {
			inline explicit strcut_node(void) noexcept = delete;
			inline explicit strcut_node(strcut_node const&) noexcept = delete;
			inline explicit strcut_node(strcut_node&&) noexcept = delete;
			inline auto operator=(strcut_node const&) noexcept = delete;
			inline auto operator=(strcut_node&&) noexcept = delete;
			inline ~strcut_node(void) noexcept = delete;
			strcut_node* _next;
			type _value;
		};
		using node = typename std::conditional<compress, union union_node, struct strcut_node>::type;
		class global final {
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
			inline explicit global(void) noexcept = default;
			inline explicit global(global const&) noexcept = delete;
			inline explicit global(global&&) noexcept = delete;
			inline auto operator=(global const&) noexcept -> global & = delete;
			inline auto operator=(global&&) noexcept -> global & = delete;
			inline ~global(void) noexcept {
				node** _node_array = reinterpret_cast<node**>(malloc(sizeof(node*) * _capacity));
				size_type _node_array_index = 0;

				bucket* head = reinterpret_cast<bucket*>(0x00007FFFFFFFFFFFULL & _head);
				while (nullptr != head) {
					bucket* next = head->_next;

					node* _node = head->_value;
					for (size_type index = 0; index < head->_size; ++index) {
						if (0 == (reinterpret_cast<uintptr_t>(_node) & (_align - 1)))
#pragma warning(suppress: 6011)
							_node_array[_node_array_index++] = _node;
						_node = _node->_next;
					}
					_pool.deallocate(*head);
					head = next;
				}

				for (size_type index = 0; index < _node_array_index; ++index)
					_aligned_free(_node_array[index]);
				free(_node_array);
			};

			inline auto allocate(void) noexcept -> pair<node*, size_type> {
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
						_pool.deallocate(*address);
						return result;
					}
				}
			}
			inline void deallocate(node* value, size_type size) noexcept {
				bucket* current = &_pool.allocate();
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
		private:
			alignas(64) unsigned long long _head = 0;
			size_type _capacity = 0;
			lockfree::pool<bucket> _pool;
		};

		inline explicit pool(void) noexcept = default;
		inline explicit pool(pool const&) noexcept = delete;
		inline explicit pool(pool&&) noexcept = delete;
		inline auto operator=(pool const&) noexcept -> pool & = delete;
		inline auto operator=(pool&&) noexcept -> pool & = delete;
		inline ~pool(void) noexcept {
			if (bucket_size < _size) {
				_global.deallocate(_head, _size - bucket_size);
				_head = _break;
				_size = bucket_size;
			}
			if (0 < _size)
				_global.deallocate(_head, _size);
		};
	public:
		template<typename... argument>
		inline auto allocate(argument&&... arg) noexcept -> type& {
			if (0 == _size) {
				auto [value, size] = _global.allocate();
				_head = value;
				_size = size;
			}
			node* current = _head;
			_head = current->_next;
			if constexpr (true == placement)
				memory::construct<type>(current->_value);
			--_size;
			return current->_value;
		}
		inline void deallocate(type& value) noexcept {
			if constexpr (true == placement)
				memory::destruct<type>(value);
			node* current = reinterpret_cast<node*>(reinterpret_cast<unsigned char*>(&value) - offsetof(node, _value));
			current->_next = _head;
			_head = current;
			++_size;

			if (bucket_size == _size)
				_break = _head;
			else if (bucket_size * 2 == _size) {
				_global.deallocate(_head, bucket_size);
				_head = _break;
				_size -= bucket_size;
			}
		}
	private:
		inline static global _global;
		node* _head = nullptr;
		node* _break = nullptr;
		size_type _size = 0;
	};
}