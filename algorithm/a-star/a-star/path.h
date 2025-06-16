#pragma once
#include "grid.h"

#include "library/list.h"
#include "library/vector.h"
#include "library/bit_grid.h"
#include "library/unordered_map.h"
#include "library/shared_pointer.h"
#include "library/weak_pointer.h"

namespace a_star {
	class path final {
	public:
		using size_type = unsigned int;
		struct node final {
		public:
			inline explicit node(coordinate const& position, library::shared_pointer<node> parent, coordinate const& destination) noexcept
				: _position(position), _parent(parent) {
				if (nullptr == _parent)
					_ground = 0.f;
				else
					_ground = _parent->_ground + _position.distance_euclidean(_parent->_position);
				_full = _position.distance_manhattan(destination) + _ground;
			}
			inline ~node(void) noexcept = default;
		public:
			inline bool operator<(node const& rhs) const noexcept {
				//if (sour->_full == dest->_full)
				//	return sour->_heuri < dest->_heuri;
				//return sour->_full < dest->_full;
				return _full < rhs._full;
			}
		public:
			float _full;
			float _ground;

			coordinate _position;
			library::shared_pointer<node> _parent;
		};
		class heap final {
		public:
			struct node final {
				path::node* _node;
				size_type _index = 0;
			};
		public:
			inline explicit heap(void) noexcept = default;
			inline ~heap(void) noexcept {
				clear();
			}
		public:
			inline void push(path::node* value) noexcept {
				auto leaf = static_cast<node*>(malloc(sizeof(node)));
				leaf->_node = value;
				_umap.emplace(value->_position, leaf);
				_vector.emplace_back(leaf);

				auto child = _vector.size() - 1;
				while (0 < child) {
					auto parent = (child - 1) / 2;

					if (*_vector[parent]->_node < *leaf->_node)
						break;
					_vector[child] = _vector[parent];
					_vector[child]->_index = child;
					child = parent;
				}
				_vector[child] = leaf;
				_vector[child]->_index = child;
			}
			inline void pop(void) noexcept {
				auto cur = _vector.front();
				_umap.erase(cur->_node->_position);

				auto leaf = _vector.back();
				auto size = _vector.size() - 1;

				size_type parent = 0;
				for (;;) {
					auto left = parent * 2 + 1;
					if (size <= left)
						break;
					auto right = left + 1;

					if (size > right && *_vector[right]->_node < *_vector[left]->_node)
						left = right;
					if (*leaf->_node < *_vector[left]->_node)
						break;

					_vector[parent] = _vector[left];
					_vector[parent]->_index = parent;
					parent = left;
				}
				_vector[parent] = leaf;
				_vector[parent]->_index = parent;
				_vector.pop_back();

				free(cur);
			}
			inline auto top(void) const noexcept -> path::node* {
				return _vector.front()->_node;
			};
			inline auto shift_up(coordinate& position, library::shared_pointer<path::node> parent) const noexcept {
				auto find = (*_umap.find(position))._second;
				float ground = parent->_ground + find->_node->_position.distance_euclidean(parent->_position);
				if (ground >= find->_node->_ground)
					return;

				find->_node->_parent = parent;
				find->_node->_full -= find->_node->_ground;
				find->_node->_full += ground;
				find->_node->_ground = ground;

				auto leaf = _vector[find->_index];
				auto child = find->_index;
				while (0 < child) {
					auto parent = (child - 1) / 2;

					if (*_vector[parent]->_node < *leaf->_node)
						break;
					_vector[child] = _vector[parent];
					_vector[child]->_index = child;
					child = parent;
				}
				_vector[child] = leaf;
				_vector[child]->_index = child;
			}
		public:
			inline void clear(void) noexcept {
				for (auto& iter : _vector) {
					delete iter->_node;
					free(iter);
				}
				_vector.clear();
				_umap.clear();
			}
			inline bool empty(void) const noexcept {
				return _vector.empty();
			}
		public:
			library::vector<node*> _vector;
			library::unordered_map<coordinate const, node*> _umap;
		};
	public:
		inline explicit path(void) noexcept
			: _grid(nullptr) {
		}
		inline explicit path(grid const& grid) noexcept
			: _grid(&grid),
			_close(grid.get_width(), grid.get_height()),
			_open(grid.get_width(), grid.get_height()) {
		}
		inline ~path(void) noexcept = default;
	public:
		inline void search(void) noexcept {
			_close.clear();
			_open.clear();
			_result.clear();
			_heap.clear();
			_parent.clear();

			_heap.push(new node(_source, nullptr, _destination));
			_open.set_bit(_source._x, _source._y, true);
			while (!_heap.empty()) {
				library::shared_pointer<node>current(_heap.top());
				_heap.pop();
				_close.set_bit(current->_position._x, current->_position._y, true);

				if (current->_position == _destination) {
					while (nullptr != current) {
						_result.emplace_front(current->_position);
						current = current->_parent;
					}
					return;
				}

				for (unsigned char direction = 0; direction < 8; ++direction) {
					static coordinate constexpr offset[] = { {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1} };
					coordinate position{ current->_position._x + offset[direction]._x, current->_position._y + offset[direction]._y };

					if (_grid->get_tile(position) || _close.get_bit(position._x, position._y))
						continue;
					switch (_open.get_bit(position._x, position._y)) {
					case false:
						_heap.push(new node(position, current, _destination));
						_open.set_bit(position._x, position._y, true);
						break;
					case true:
						_heap.shift_up(position, current);
						break;
					}
				}
				_parent.emplace_back(current);
			}
		}
		inline auto result(void) noexcept -> library::list<coordinate>& {
			return _result;
		}
	public:
		inline void set_grid(grid const& grid) noexcept {
			_grid = &grid;
			_close.set_size(grid.get_width(), grid.get_height());
			_open.set_size(grid.get_width(), grid.get_height());
		}
		inline void set_source(coordinate const& position) noexcept {
			_source = position;
		}
		inline void set_destination(coordinate const& position) noexcept {
			_destination = position;
		}
	public:
		library::bit_grid<long long> _close;
		library::bit_grid<long long> _open;
		heap _heap;
		library::list<library::weak_pointer<node>> _parent;

		grid const* _grid;
		coordinate _source{};
		coordinate _destination{};
		library::list<coordinate> _result;
		library::list<coordinate> _bresenham;
	};
}