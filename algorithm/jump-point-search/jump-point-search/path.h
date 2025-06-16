#pragma once
#include "coordinate.h"
#include "grid.h"

#include "../../../data-structure/list/list.h"
#include "../../../data-structure/vector/vector.h"
#include "../../../data-structure/bit-grid/bit_grid.h"
#include "../../../data-structure/unordered-map/unordered_map.h"
#include "../../../data-structure/shared-pointer/shared_pointer.h"
#include "../../../data-structure/weak-pointer/weak_pointer.h"
#include <optional>

namespace algorithm::jump_point_search {
	class path final {
	public:
		using size_type = unsigned int;
		struct node final {
		public:
			inline explicit node(coordinate const& position, direction dir, library::data_structure::shared_pointer<node> parent, coordinate const& destination) noexcept
				: _position(position), _direction(dir), _parent(parent) {
				if (nullptr == _parent)
					_ground = 0;
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
			direction _direction;
			library::data_structure::shared_pointer<node> _parent;
		};
		class heap final {
		public:
			struct node final {
				path::node* _node;
				size_type _index = 0;
			};
		public:
			inline explicit heap(void) noexcept {
			}
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

					if (_vector[parent]->_node->_full < leaf->_node->_full)
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
			inline auto shift_up(coordinate& position, library::data_structure::shared_pointer<path::node> parent) const noexcept {
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

					if (_vector[parent]->_node->_full < leaf->_node->_full)
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
			library::data_structure::vector<node*> _vector;
			library::data_structure::unordered_map<coordinate const, node*> _umap;
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

			_heap.push(new node(_source, direction::all, nullptr, _destination));
			_open.set_bit(_source._x, _source._y, true);

			while (!_heap.empty()) {
				library::data_structure::shared_pointer<node>current(_heap.top());
				_heap.pop();
				_close.set_bit(current->_position._x, current->_position._y, true);

				if (current->_position == _destination) {
					while (nullptr != current) {
						_result.emplace_front(current->_position);
						current = current->_parent;
					}
					return;
				}
				bit_direction bit_dir = natural_neighbor(current->_direction) | force_neighbor(current->_position, current->_direction);

				for (unsigned char dir = 0; dir < 8; ++dir) {
					if (false == ((1 << dir) & bit_dir))
						continue;
					std::optional<coordinate> position = search_direction(current->_position, static_cast<direction>(dir));
					if (!position)
						continue;
					if (_close.get_bit(position->_x, position->_y))
						continue;
					switch (_open.get_bit(position->_x, position->_y)) {
					case false:
						_heap.push(new node(*position, static_cast<direction>(dir), current, _destination));
						_open.set_bit(position->_x, position->_y, true);
						break;
					case true:
						_heap.shift_up(*position, current);
						break;
					}
				}
				_parent.emplace_back(current);
			}
		};
		inline auto result(void) noexcept -> library::data_structure::list<coordinate>& {
			return _result;
		}
	public:
		inline auto search_direction(coordinate pos, direction const dir) noexcept -> std::optional<coordinate> {
			auto diag = is_diagonal(dir);
			for (;;) {
				pos = move_position(pos, dir);
				if (is_impassible(pos))
					return std::nullopt;
				if (_destination == pos)
					return pos;
				switch (diag) {
				case false:
					return search_straight(pos, dir);
				case true:
					if (force_neighbor(pos, dir))
						return pos;
					{
						auto rot = rotate_direction(dir, 1);
						if (search_straight(move_position(pos, rot), rot))
							return pos;
					}
					{
						auto rot = rotate_direction(dir, 7);
						if (search_straight(move_position(pos, rot), rot))
							return pos;
					}
					break;
				}
			}
		};
		inline auto search_straight(coordinate pos, direction const dir) noexcept -> std::optional<coordinate> {
			if (is_impassible(pos))
				return std::nullopt;
			auto axi = dir == left || dir == right ? horizontal : vertical;
			auto move = dir == left || dir == up ? backward : forward;

			auto top = search_side(move_position(pos, rotate_direction(dir, 6)), axi, move);
			auto center = jump_point(pos, axi, move, true);
			auto bottom = search_side(move_position(pos, rotate_direction(dir, 2)), axi, move);

			std::optional<coordinate> result = std::nullopt;
			switch (dir) {
			case up:
				if (pos._x == _destination._x && pos._y >= _destination._y && (!center || *center < _destination._y))
					return _destination;
				if (top && (!center || *center <= *top))
					result = { pos._x,  *top + 1 };
				if (bottom && (!center || *center <= *bottom))
					if (!result || *bottom > *top)
						result = { pos._x, *bottom + 1 };
				break;
			case right:
				if (pos._y == _destination._y && pos._x <= _destination._x && (!center || *center > _destination._x))
					return _destination;
				if (top && (!center || *center >= *top))
					result = { *top - 1, pos._y };
				if (bottom && (!center || *center >= *bottom))
					if (!result || *bottom < *top)
						result = { *bottom - 1, pos._y };
				break;
			case down:
				if (pos._x == _destination._x && pos._y <= _destination._y && (!center || *center > _destination._y))
					return _destination;
				if (top && (!center || *center >= *top))
					result = { pos._x,  *top - 1 };
				if (bottom && (!center || *center >= *bottom))
					if (!result || *bottom < *top)
						result = { pos._x,  *bottom - 1 };
				break;
			case left:
				if (pos._y == _destination._y && pos._x >= _destination._x && (!center || *center < _destination._x))
					return _destination;
				if (top && (!center || *center <= *top))
					result = { *top + 1, pos._y };
				if (bottom && (!center || *center <= *bottom))
					if (!result || *bottom > *top)
						result = { *bottom + 1, pos._y };
				break;
			}

			return result;
		};
		inline auto search_side(coordinate pos, axis const axi, movement const move) -> std::optional<size_type> {
			if (!in_bound(pos))
				return std::nullopt;
			auto close = jump_point(pos, axi, move, true);
			if (!close)
				return std::nullopt;

			if (horizontal == axi)
				return jump_point(coordinate{ *close, pos._y }, axi, move, false);
			return jump_point(coordinate{ pos._x, *close }, axi, move, false);
		}
		inline auto jump_point(coordinate const pos, axis const axi, movement const move, bool const flag) noexcept -> std::optional<size_type> {
			auto sour = _grid->get_div(pos, axi);
			auto dest = _grid->get_div(pos, axi, move);

			auto step = backward == move ? -1 : 1;
			for (auto cur = sour;; cur.quot += step) {
				auto word = _grid->get_word(cur.quot, axi);
				word = false == flag ? ~word : word;

				mask_word(cur.quot, sour, word, move);
				mask_word(cur.quot, dest, word, static_cast<movement>(!move));

				if (scan_word(cur.rem, word, move))
					return _grid->get_position(cur, axi)._x;
				if (cur.quot == dest.quot)
					break;
			}
			return std::nullopt;
		}
		inline void mask_word(long long const idx, div_t const& div, unsigned long long& word, movement const move) noexcept {
			static long long constexpr mask[][64] = {
				{ 1ULL,					3ULL,					7ULL,					15ULL,
				31ULL,					63ULL,					127ULL,					255ULL,
				511ULL,					1023ULL,				2047ULL,				4095ULL,
				8191ULL,				16383ULL,				32767ULL,				65535ULL,
				131071ULL,				262143ULL,				524287ULL,				1048575ULL,
				2097151ULL,				4194303ULL,				8388607ULL,				16777215ULL,
				33554431ULL,			67108863ULL,			134217727ULL,			268435455ULL,
				536870911ULL,			1073741823ULL,			2147483647ULL,			4294967295ULL,
				8589934591ULL,			17179869183ULL,			34359738367ULL,			68719476735ULL,
				137438953471ULL,		274877906943ULL,		549755813887ULL,		1099511627775ULL,
				2199023255551ULL,		4398046511103ULL,		8796093022207ULL,		17592186044415ULL,
				35184372088831ULL,		70368744177663ULL,		140737488355327ULL,		281474976710655ULL,
				562949953421311ULL,		1125899906842623ULL,	2251799813685247ULL,	4503599627370495ULL,
				9007199254740991ULL,	18014398509481983ULL,	36028797018963967ULL,	72057594037927935ULL,
				144115188075855871ULL,	288230376151711743ULL,	576460752303423487ULL,	1152921504606846975ULL,
				2305843009213693951ULL,	4611686018427387903ULL,	9223372036854775807ULL,	18446744073709551615ULL },
				{ 18446744073709551615ULL, 18446744073709551614ULL, 18446744073709551612ULL, 18446744073709551608ULL,
				18446744073709551600ULL, 18446744073709551584ULL, 18446744073709551552ULL, 18446744073709551488ULL,
				18446744073709551360ULL, 18446744073709551104ULL, 18446744073709550592ULL, 18446744073709549568ULL,
				18446744073709547520ULL, 18446744073709543424ULL, 18446744073709535232ULL, 18446744073709518848ULL,
				18446744073709486080ULL, 18446744073709420544ULL, 18446744073709289472ULL, 18446744073709027328ULL,
				18446744073708503040ULL, 18446744073707454464ULL, 18446744073705357312ULL, 18446744073701163008ULL,
				18446744073692774400ULL, 18446744073675997184ULL, 18446744073642442752ULL, 18446744073575333888ULL,
				18446744073441116160ULL, 18446744073172680704ULL, 18446744072635809792ULL, 18446744071562067968ULL,
				18446744069414584320ULL, 18446744065119617024ULL, 18446744056529682432ULL, 18446744039349813248ULL,
				18446744004990074880ULL, 18446743936270598144ULL, 18446743798831644672ULL, 18446743523953737728ULL,
				18446742974197923840ULL, 18446741874686296064ULL, 18446739675663040512ULL, 18446735277616529408ULL,
				18446726481523507200ULL, 18446708889337462784ULL, 18446673704965373952ULL, 18446603336221196288ULL,
				18446462598732840960ULL, 18446181123756130304ULL, 18445618173802708992ULL, 18444492273895866368ULL,
				18442240474082181120ULL, 18437736874454810624ULL, 18428729675200069632ULL, 18410715276690587648ULL,
				18374686479671623680ULL, 18302628885633695744ULL, 18158513697557839872ULL, 17870283321406128128ULL,
				17293822569102704640ULL, 16140901064495857664ULL, 13835058055282163712ULL, 9223372036854775808ULL }
			};
			if (idx == div.quot)
				word &= mask[move][div.rem];
		}
		inline bool scan_word(int& idx, long long word, movement const move) noexcept {
			static long long constexpr scan[][6] = {
				{ 0xffffffff00000000ULL, 0xffff000000000000ULL, 0xff00000000000000ULL,
				0xf000000000000000ULL, 0xc000000000000000ULL, 0x8000000000000000ULL },
				{ 0xffffffffULL, 0xffffULL, 0xffULL,
				0xfULL, 0x3ULL, 0x1ULL },
			};
			if (!word)
				return false;
			idx = backward == move ? 63 : 0;
			unsigned char num = 32;
			for (int i = 0; i < 6; ++i) {
				if (!(word & scan[move][i]))
					switch (move) {
					case backward:
						idx -= num;
						word <<= num;
						break;
					case forward:
						idx += num;
						word >>= num;
						break;
					}
				num /= 2;
			}
			return true;
		}
	public:
		inline auto move_position(coordinate const& position, direction const dir) const noexcept -> coordinate {
			static coordinate constexpr offset[] = { {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1} };
			return coordinate{ position._x + offset[dir]._x, position._y + offset[dir]._y };
		}
		inline auto rotate_direction(direction const dir, unsigned char const rot) const noexcept -> direction {
			return static_cast<direction>((dir + rot) % 8);
		}
		inline bool is_diagonal(direction const dir) const noexcept {
			return 0 == dir % 2 ? false : true;
		}
		inline bool is_block(coordinate const& pos) const noexcept {
			return _grid->get_tile(pos);
		}
		inline bool in_bound(coordinate const& pos) const noexcept {
			return _grid->in_bound(pos);
		}
		inline bool is_impassible(coordinate const& pos) const noexcept {
			return !in_bound(pos) || is_block(pos);
		}
		inline auto natural_neighbor(direction const dir) const noexcept -> bit_direction {
			if (8 == dir)
				return 255;
			bit_direction bit = 0;
			bit |= 1 << dir;
			if (is_diagonal(dir)) {
				bit |= 1 << (dir + 1) % 8;
				bit |= 1 << (dir + 7) % 8;
			}
			return bit;
		}
		inline auto force_neighbor(coordinate const& pos, direction const dir) const noexcept -> bit_direction {
			if (8 == dir)
				return 0;
			bit_direction bit = 0;
			if (is_diagonal(dir)) {
				if (is_impassible(move_position(pos, rotate_direction(dir, 5))) &&
					!is_impassible(move_position(pos, rotate_direction(dir, 6))))
					bit |= 1 << (dir + 6) % 8;
				if (is_impassible(move_position(pos, rotate_direction(dir, 3))) &&
					!is_impassible(move_position(pos, rotate_direction(dir, 2))))
					bit |= 1 << (dir + 2) % 8;
			}
			else {
				if (is_impassible(move_position(pos, rotate_direction(dir, 6))) &&
					!is_impassible(move_position(pos, rotate_direction(dir, 7))))
					bit |= 1 << (dir + 7) % 8;
				if (is_impassible(move_position(pos, rotate_direction(dir, 2))) &&
					!is_impassible(move_position(pos, rotate_direction(dir, 1))))
					bit |= 1 << (dir + 1) % 8;
			}
			return bit;
		}
	public:
		inline void set_source(coordinate const& position) noexcept {
			_source = position;
		}
		inline void set_destination(coordinate const& position) noexcept {
			_destination = position;
		}
	public:
		library::data_structure::bit_grid<long long> _close;
		library::data_structure::bit_grid<long long> _open;
		heap _heap;
		library::data_structure::list<library::data_structure::weak_pointer<node>> _parent;

		grid const* _grid;
		coordinate _source{};
		coordinate _destination{};
		library::data_structure::list<coordinate> _result;
		library::data_structure::list<coordinate> _bresenham;
	};
}


//inline bool assist(coordinate const& pos, direction const dir, unsigned char rot) noexcept {
//	auto rotate = rotate_direction(static_cast<direction>(dir), rot);
//	auto position = move_position(pos, rotate);
//	if (!_grid->in_bound(position) || _grid->get_tile(position))
//		return false;
//	if (straight(position, rotate))
//		return true;
//	return false;
//}