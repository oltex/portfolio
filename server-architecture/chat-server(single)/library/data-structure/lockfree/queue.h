#pragma once
#include "../thread-local/memory_pool.h"
#include <optional>

namespace data_structure::lockfree {
	template <typename type>
		requires std::is_trivially_copy_constructible_v<type>&& std::is_trivially_destructible_v<type>
	class queue {
	protected:
		using size_type = unsigned int;
		struct node final {
			inline explicit node(void) noexcept = delete;
			inline explicit node(node const&) noexcept = delete;
			inline explicit node(node&&) noexcept = delete;
			inline auto operator=(node const&) noexcept = delete;
			inline auto operator=(node&&) noexcept = delete;
			inline ~node(void) noexcept = delete;
			unsigned long long _next;
			type _value;
		};
		using _memory_pool = data_structure::_thread_local::memory_pool<node>;
	public:
		inline explicit queue(void) noexcept {
			node* current = &_memory_pool::instance().allocate();
			current->_next = _nullptr = _InterlockedIncrement(&_static_nullptr);
			_head = _tail = reinterpret_cast<unsigned long long>(current);
		}
		inline explicit queue(queue const& rhs) noexcept = delete;
		inline explicit queue(queue&& rhs) noexcept = delete;
		inline auto operator=(queue const& rhs) noexcept -> queue & = delete;
		inline auto operator=(queue&& rhs) noexcept -> queue & = delete;
		inline ~queue(void) noexcept {
			node* head = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & _head);
			while (_nullptr != reinterpret_cast<unsigned long long>(head)) {
				node* next = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head->_next);
				if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
					head->_value.~type();
				_memory_pool::instance().deallocate(*head);
				head = next;
			}
		};

		template<typename universal>
		inline void push(universal&& value) noexcept {
			emplace(std::forward<universal>(value));
		}
		template<typename... argument>
		inline void emplace(argument&&... arg) noexcept {
			node* current = &_memory_pool::instance().allocate();
			if constexpr (std::is_class_v<type>) {
				if constexpr (std::is_constructible_v<type, argument...>)
					::new(reinterpret_cast<void*>(&current->_value)) type(std::forward<argument>(arg)...);
			}
			else if constexpr (1 == sizeof...(arg))
#pragma warning(suppress: 6011)
				current->_value = type(std::forward<argument>(arg)...);

			for (;;) {
				unsigned long long tail = _tail;
				unsigned long long count = 0xFFFF800000000000ULL & tail;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & tail);
				unsigned long long next = address->_next;

				if (0x10000 <= (0x00007FFFFFFFFFFFULL & next))
					_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);
				else if (_nullptr == (0x00007FFFFFFFFFFFULL & next) && count == (0xFFFF800000000000ULL & next)) {
					unsigned long long next_count = count + 0x0000800000000000ULL;
					unsigned long long next_tail = reinterpret_cast<unsigned long long>(current) + next_count;
					current->_next = next_count + _nullptr;
					if (next == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&address->_next), next_tail, next))
						break;
				}
			}
		}
		inline auto pop(void) noexcept -> std::optional<type> {
			for (;;) {
				unsigned long long head = _head;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & head);
				unsigned long long next = address->_next;

				if (_nullptr == (0x00007FFFFFFFFFFFULL & next))
					return std::nullopt;
				else if (0x10000 <= (0x00007FFFFFFFFFFFULL & next)) {
					unsigned long long tail = _tail;
					if (tail == head)
						_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);
					type result = reinterpret_cast<node*>(0x00007FFFFFFFFFFFULL & next)->_value;
					if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head)) {
						if constexpr (std::is_destructible_v<type> && !std::is_trivially_destructible_v<type>)
							address->_value.~type();
						_memory_pool::instance().deallocate(*address);
						return result;
					}
				}
			}
		}
	protected:
		unsigned long long _head;
		unsigned long long _tail;
		unsigned long long _nullptr;
		inline static unsigned long long _static_nullptr = 0;
	};
}