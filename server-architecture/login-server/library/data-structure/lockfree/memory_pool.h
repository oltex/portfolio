#pragma once
#include <memory>
#include <Windows.h>

namespace data_structure::lockfree {
	template<typename type>
	class memory_pool final {
	private:
		union node final {
			inline explicit node(void) noexcept = delete;
			inline explicit node(node const&) noexcept = delete;
			inline explicit node(node&&) noexcept = delete;
			inline auto operator=(node const&) noexcept = delete;
			inline auto operator=(node&&) noexcept = delete;
			inline ~node(void) noexcept = delete;
			node* _next;
			type _value;
		};
	public:
		inline explicit memory_pool(void) noexcept
			: _head(0) {
		};
		inline explicit memory_pool(memory_pool const&) noexcept = delete;
		inline explicit memory_pool(memory_pool&& rhs) noexcept
			: _head(rhs._head) {
			rhs._head = nullptr;
		};
		inline auto operator=(memory_pool const&) noexcept = delete;
		inline auto operator=(memory_pool&& rhs) noexcept {
			_head = rhs._head;
			rhs._head = nullptr;
		}
		inline ~memory_pool(void) noexcept {
			node* head = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & _head);
			while (nullptr != head) {
				node* next = head->_next;
				free(head);
				head = next;
			}
		}
	public:
		template<typename... argument>
		inline auto allocate(argument&&... arg) noexcept -> type& {
			node* current;
			for (;;) {
				unsigned long long head = _head;
				current = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				if (nullptr == current) {
					current = reinterpret_cast<node*>(malloc(sizeof(node)));
					break;
				}
				unsigned long long next = reinterpret_cast<unsigned long long>(current->_next) + (0xFFFF800000000000ULL & head) + 0x0000800000000000ULL;
				if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head))
					break;
			}

			if constexpr (std::is_class_v<type>) {
				if constexpr (std::is_constructible_v<type, argument...>)
					::new(reinterpret_cast<void*>(&current->_value)) type(std::forward<argument>(arg)...);
			}
			else if constexpr (1 == sizeof...(arg))
				current->_value = type(std::forward<argument>(arg)...);
#pragma warning(suppress: 6011)
			return current->_value;
		}
		inline void deallocate(type& value) noexcept {
			if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
				value.~type();

			for (;;) {
				unsigned long long head = _head;
				reinterpret_cast<node&>(value)._next = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				unsigned long long next = reinterpret_cast<unsigned long long>(&value) + (0xFFFF800000000000ULL & head) + 0x0000800000000000ULL;
				if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head))
					break;
			}
		}
	private:
		unsigned long long _head;
	};
}