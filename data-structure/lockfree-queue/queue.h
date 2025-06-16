#pragma once
#include "../../../system/memory/memory.h"
#include "../../thread-local/pool/pool.h"
#include <optional>

namespace library::data_structure::lockfree {
	template <typename type, bool multi_pop = true>
		requires std::is_trivially_copy_constructible_v<type> && std::is_trivially_destructible_v<type>
	class queue {
	protected:
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
		using _pool = _thread_local::pool<node>;
	public:
		inline explicit queue(void) noexcept {
			node* current = &_pool::instance().allocate();
			current->_next = reinterpret_cast<unsigned long long>(this);
			_head = _tail = reinterpret_cast<unsigned long long>(current) + 1;
		}
		inline explicit queue(queue const&) noexcept = delete;
		inline explicit queue(queue&&) noexcept = delete;
		inline auto operator=(queue const&) noexcept -> queue & = delete;
		inline auto operator=(queue&&) noexcept -> queue & = delete;
		inline ~queue(void) noexcept {
			node* head = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & _head);
			while (reinterpret_cast<unsigned long long>(this) != reinterpret_cast<unsigned long long>(head)) {
				node* next = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & head->_next);
				_pool::instance().deallocate(*head);
				head = next;
			}
		};

		template<typename... argument>
		inline void emplace(argument&&... arg) noexcept {
			node* current = &_pool::instance().allocate();
			system::memory::construct<type>(current->_value, std::forward<argument>(arg)...);

			for (;;) {
				unsigned long long tail = _tail;
				unsigned long long count = 0xFFFF800000000000ULL & tail;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & tail);
				unsigned long long next = address->_next;

				if (1 == (0x1ULL & next))
					_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);
				else if (reinterpret_cast<unsigned long long>(this) == (0x00007FFFFFFFFFFFULL & next) && count == (0xFFFF800000000000ULL & next)) {
					unsigned long long next_count = count + 0x0000800000000000ULL;
					unsigned long long next_tail = reinterpret_cast<unsigned long long>(current) + next_count + 1;
					current->_next = next_count + reinterpret_cast<unsigned long long>(this);
					if (next == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&address->_next), next_tail, next))
						break;
				}
			}
		}
		inline auto pop(void) noexcept -> std::optional<type> requires (true == multi_pop) {
			for (;;) {
				unsigned long long head = _head;
				unsigned long long count = 0xFFFF800000000000ULL & head;
				node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & head);
				unsigned long long next = address->_next;

				if (reinterpret_cast<unsigned long long>(this) == (0x00007FFFFFFFFFFFULL & next) && count == (0xFFFF800000000000ULL & next))
					return std::nullopt;
				else if (1 == (0x1ULL & next)) {
					unsigned long long tail = _tail;
					if (tail == head)
						_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);
					type result = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & next)->_value;
					if (head == _InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_head), next, head)) {
						_pool::instance().deallocate(*address);
						return result;
					}
				}
			}
		}
		inline auto pop(void) noexcept -> type requires (false == multi_pop) {
			unsigned long long head = _head;
			node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & head);
			unsigned long long next = address->_next;

			if (0 == (0x1ULL & next))
				__debugbreak();
			unsigned long long tail = _tail;
			if (tail == head)
				_InterlockedCompareExchange(reinterpret_cast<unsigned long long volatile*>(&_tail), next, tail);

			type result = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & next)->_value;
			_head = next;
			_pool::instance().deallocate(*address);
			return result;
		}
		inline auto empty(void) const noexcept requires (false == multi_pop) {
			unsigned long long head = _head;
			unsigned long long count = 0xFFFF800000000000ULL & head;
			node* address = reinterpret_cast<node*>(0x00007FFFFFFFFFFEULL & head);
			unsigned long long next = address->_next;
			if (reinterpret_cast<unsigned long long>(this) == (0x00007FFFFFFFFFFFULL & next) && count == (0xFFFF800000000000ULL & next))
				return true;
			return false;
		}
	protected:
		alignas(64) unsigned long long _head;
		alignas(64) unsigned long long _tail;
	};
}