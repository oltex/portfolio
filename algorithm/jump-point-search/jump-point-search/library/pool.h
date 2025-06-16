#pragma once
#include "memory.h"

namespace library {
	template<typename type, bool placement = true, bool compress = true>
	class pool final {
	private:
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
		using node = typename std::conditional<compress, union union_node, struct strcut_node>::type;
	public:
		template <typename other>
		using rebind = pool<other>;

		inline explicit pool(void) noexcept = default;
		inline explicit pool(pool const&) noexcept = delete;
		inline explicit pool(pool&& rhs) noexcept
			: _head(rhs._head) {
			rhs._head = nullptr;
		};
		inline auto operator=(pool const&) noexcept = delete;
		inline auto operator=(pool&& rhs) noexcept {
			_head = rhs._head;
			rhs._head = nullptr;
		}
		inline ~pool(void) noexcept {
			while (nullptr != _head) {
#pragma warning(suppress: 6001)
				node* next = _head->_next;
				memory::deallocate<node>(_head);
				_head = next;
			}
		}

		template<typename... argument>
		inline auto allocate(argument&&... arg) noexcept -> type& {
			node* current;
			if (nullptr == _head)
				current = memory::allocate<node>();
			else {
				current = _head;
				_head = current->_next;
			}
			if constexpr (true == placement)
				memory::construct<type>(current->_value, std::forward<argument>(arg)...);
			return current->_value;
		}
		inline void deallocate(type& value) noexcept {
			if constexpr (true == placement)
				memory::destruct<type>(value);
			node* current = reinterpret_cast<node*>(reinterpret_cast<unsigned char*>(&value) - offsetof(node, _value));
			current->_next = _head;
			_head = current;
		}
	private:
		node* _head = nullptr;
	};
}