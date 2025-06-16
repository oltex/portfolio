#pragma once
#include "../../../system/memory/memory.h"
#include "../../thread-local/pool/pool.h"

namespace library::data_structure {
	template<typename type>
	class queue {
	private:
		struct node final {
			inline explicit node(void) noexcept = delete;
			inline explicit node(node const&) noexcept = delete;
			inline explicit node(node&&) noexcept = delete;
			inline auto operator=(node const&) noexcept = delete;
			inline auto operator=(node&&) noexcept = delete;
			inline ~node(void) noexcept = delete;
			node* _next;
			type _value;
		};
		using _pool = _thread_local::pool<node>;
	public:
		inline queue(void) noexcept {
			node* current = &_pool::instance().allocate();
			current->_next = nullptr;
			_head = _tail = current;
		}
		inline ~queue(void) noexcept = default;

		template<typename... argument>
		inline void push(argument&&... arg) noexcept {
			node* current = &_pool::instance().allocate();
			system::memory::construct<type>(current->_value, std::forward<argument>(arg)...);

			current->_next = nullptr;
			_tail->_next = current;
			_tail = current;
		}
		inline auto pop(void) noexcept -> type {
			node* current = _head;

			type result = current->_next->_value;
			_head = current->_next;
			_pool::instance().deallocate(*current);

			return result;
		}
	private:
		alignas(64) node* _head;
		alignas(64) node* _tail;
	};
}