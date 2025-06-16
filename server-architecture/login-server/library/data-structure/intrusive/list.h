#pragma once
#include <utility>
#include <stdlib.h>
#include <malloc.h>

namespace data_structure::intrusive {
	template<size_t index>
	class list_hook {
	private:
		template<typename type, size_t>
		friend class list;
	private:
		list_hook* _prev, * _next;
	};

	template<typename type, size_t index>
	//requires std::is_base_of<list_hook<index>, type>::value
	class list final {
	private:
		using size_type = unsigned int;
		using node = list_hook<index>;
		static_assert(std::is_base_of<node, type>::value);
	public:
		class iterator final {
		public:
			inline explicit iterator(node* const node = nullptr) noexcept
				: _node(node) {
			}
			inline iterator(iterator const& rhs) noexcept
				: _node(rhs._node) {
			}
			inline auto operator=(iterator const& rhs) noexcept -> iterator& {
				_node = rhs._node;
				return *this;
			}
		public:
			inline auto operator*(void) const noexcept -> type& {
				return static_cast<type&>(*_node);
			}
			inline auto operator->(void) noexcept -> type* {
				return static_cast<type*>(_node);
			}
			inline auto operator++(void) noexcept -> iterator& {
				_node = _node->_next;
				return *this;
			}
			inline auto operator++(int) noexcept -> iterator {
				iterator iter(*this);
				_node = _node->_next;
				return iter;
			}
			inline auto operator--(void) noexcept -> iterator& {
				_node = _node->_prev;
				return *this;
			}
			inline auto operator--(int) noexcept -> iterator {
				iterator iter(*this);
				_node = _node->_prev;
				return iter;
			}
			inline bool operator==(iterator const& rhs) const noexcept {
				return _node == rhs._node;
			}
			inline bool operator!=(iterator const& rhs) const noexcept {
				return _node != rhs._node;
			}
		public:
			node* _node;
		};
	public:
		inline explicit list(void) noexcept {
			_head._next = _head._prev = &_head;
		}
		inline ~list(void) noexcept = default;
	public:
		inline void push_front(type& value) noexcept {
			insert(begin(), value);
		}
		inline void push_back(type& value) noexcept {
			insert(end(), value);
		}
		inline auto insert(iterator const& iter, type& value) noexcept -> iterator {
			auto current = static_cast<node*>(&value);
			auto next = iter._node;
			auto prev = next->_prev;

			prev->_next = current;
			current->_prev = prev;
			current->_next = next;
			next->_prev = current;

			++_size;
			return iterator(current);
		}
		inline void pop_front(void) noexcept {
			erase(begin());
		}
		inline void pop_back(void) noexcept {
			erase(--end());
		}
		inline auto erase(iterator const& iter) noexcept -> iterator {
			return erase(static_cast<type&>(*iter._node));
		}
		inline auto erase(type& value) noexcept -> iterator {
			auto current = static_cast<node*>(&value);
			auto prev = current->_prev;
			auto next = current->_next;

			prev->_next = next;
			next->_prev = prev;

			--_size;
			return iterator(next);
		}
	public:
		inline auto front(void) const noexcept -> type& {
			return static_cast<type&>(*_head._next);
		}
		inline auto back(void) const noexcept -> type& {
			return static_cast<type&>(*_head._prev);
		}
		inline auto begin(void) const noexcept -> iterator {
			return iterator(_head._next);
		}
		inline auto end(void) noexcept -> iterator {
			return iterator(&_head);
		}
	public:
		inline void swap(list& rhs) noexcept {
			node* next;
			next = _head._next;
			next->_prev = _head._prev->_next = &rhs._head;
			next = rhs._head._next;
			next->_prev = rhs._head._prev->_next = &_head;

			node head = _head;
			_head = rhs._head;
			rhs._head = head;

			size_type size = _size;
			_size = rhs._size;
			rhs._size = size;
		}
		inline void clear(void) noexcept {
			_head._next = _head._prev = &_head;
			_size = 0;
		}
		inline auto size(void) const noexcept -> size_type {
			return _size;
		}
		inline bool empty(void) const noexcept {
			return 0 == _size;
		}
	private:
		node _head;
		size_type _size = 0;
	};
}