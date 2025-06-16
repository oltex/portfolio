#pragma once
#include "jump-point-search/grid.h"
#include "jump-point-search//path.h"

class test final {
private:
	using size_type = unsigned int;
public:
	enum class type : unsigned char {
		shot, step, play
	};
	enum class state : unsigned char {
		start, run, goal
	};
public:
	inline explicit test(void) noexcept {
		QueryPerformanceFrequency(&_frequency);
		srand(static_cast<unsigned int>(8342789327));
	}
	inline ~test(void) noexcept = default;
public:
	inline void set(WPARAM const wparam, HWND hwnd, jump_point_search::grid& grid, jump_point_search::path& path) noexcept {
		static bool timer = false;
		switch (wparam) {
		case 0x31: {
			_type = type::shot;
			path.search();
			KillTimer(hwnd, 1);
			timer = false;
		} break;
		case 0x32: {
			if (type::step != _type)
				_state = state::start;
			_type = type::step;
			step(grid, path);
			KillTimer(hwnd, 1);
			timer = false;
		} break;
		case 0x33: {
			_type = type::play;
			if (false == timer) {
				SetTimer(hwnd, 1, 0, nullptr);
				_state = state::start;
				timer = true;
			}
			else {
				KillTimer(hwnd, 1);
				timer = false;
			}
		} break;
		}
	}
	inline void step(jump_point_search::grid& grid, jump_point_search::path& path) noexcept {
		switch (_state) {
		case state::start:
			_state = test::state::run;
			path._close.clear();
			path._open.clear();
			path._result.clear();
			path._heap.clear();
			path._parent.clear();
			path._heap.push(new jump_point_search::path::node(path._source, jump_point_search::direction::all, nullptr, path._destination));
			path._open.set_bit(path._source._x, path._source._y, true);
			break;
		case state::run:
			if (!path._heap.empty()) {
				library::shared_pointer<jump_point_search::path::node>current(path._heap.top());
				path._heap.pop();
				path._close.set_bit(current->_position._x, current->_position._y, true);

				if (current->_position == path._destination) {
					while (nullptr != current) {
						path._result.emplace_front(current->_position);
						current = current->_parent;
					}
					_state = test::state::start;
					break;
				}
				jump_point_search::bit_direction bit_dir = path.natural_neighbor(current->_direction) | path.force_neighbor(current->_position, current->_direction);

				for (unsigned char dir = 0; dir < 8; ++dir) {
					if (false == ((1 << dir) & bit_dir))
						continue;
					std::optional<jump_point_search::coordinate> position = path.search_direction(current->_position, static_cast<jump_point_search::direction>(dir));
					if (!position)
						continue;
					if (path._close.get_bit(position->_x, position->_y))
						continue;
					switch (path._open.get_bit(position->_x, position->_y)) {
					case false:
						path._heap.push(new jump_point_search::path::node(*position, static_cast<jump_point_search::direction>(dir), current, path._destination));
						path._open.set_bit(position->_x, position->_y, true);
						break;
					case true:
						path._heap.shift_up(*position, current);
						break;
					}
				}
				path._parent.emplace_back(current);
			}
			else {
				_state = test::state::start;
				break;
			}
			break;
		}
	}
	inline void run(jump_point_search::grid& grid, jump_point_search::path& path) noexcept {
		static size_type wait;
		switch (_state) {
		case state::start:
			_state = test::state::run;
			wait = 0;

			//grid initialize
			grid.clear();
			for (size_type y = 0; y < grid._height; ++y)
				for (size_type x = 0; x < grid._width; ++x)
					grid.set_tile(jump_point_search::coordinate{ x, y }, _wall_probability > std::rand() / (float)RAND_MAX ? true : false);
			_repeat = (rand() % 1) + 1;
			for (size_type r = 0; r < _repeat; ++r) {
				for (size_type y = 0; y < grid._height; ++y) {
					for (size_type x = 0; x < grid._width; ++x) {
						unsigned char count = 0;
						for (unsigned char i = 0; i < 8; ++i) {
							static char constexpr offset[][2] = { {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1} };
							size_type nx = x + offset[i][0];
							size_type ny = y + offset[i][1];
							if (nx >= 0 && ny >= 0 && nx < grid._width && ny < grid._height)
								count += grid.get_tile(jump_point_search::coordinate{ nx, ny });
							else
								count++;
						}
						if (4 < count)
							grid.set_tile(jump_point_search::coordinate{ x, y }, true);
						else if (4 > count)
							grid.set_tile(jump_point_search::coordinate{ x, y }, false);
					}
				}
			}
			//path initialize
			path._close.clear();
			path._open.clear();
			path._result.clear();
			path._heap.clear();
			path._parent.clear();
			for (;;) {
				jump_point_search::coordinate position = { rand() % grid._width, rand() % grid._height };
				if (false == grid.get_tile(position)) {
					path.set_source(position);
					break;
				}
			}
			for (;;) {
				jump_point_search::coordinate position = { rand() % grid._width, rand() % grid._height };
				if (false == grid.get_tile(position)) {
					path.set_destination(position);
					break;
				}
			}
			path._heap.push(new jump_point_search::path::node(path._source, jump_point_search::direction::all, nullptr, path._destination));
			path._open.set_bit(path._source._x, path._source._y, true);
			break;
		case state::run:
			for (size_type skip = 0; skip < _skip; ++skip) {
				if (!path._heap.empty()) {
					library::shared_pointer<jump_point_search::path::node>current(path._heap.top());
					path._heap.pop();
					path._close.set_bit(current->_position._x, current->_position._y, true);

					if (current->_position == path._destination) {
						while (nullptr != current) {
							path._result.emplace_front(current->_position);
							current = current->_parent;
						}
						_state = test::state::goal;
						break;
					}
					jump_point_search::bit_direction bit_dir = path.natural_neighbor(current->_direction) | path.force_neighbor(current->_position, current->_direction);

					for (unsigned char dir = 0; dir < 8; ++dir) {
						if (false == ((1 << dir) & bit_dir))
							continue;
						std::optional<jump_point_search::coordinate> position = path.search_direction(current->_position, static_cast<jump_point_search::direction>(dir));
						if (!position)
							continue;
						if (path._close.get_bit(position->_x, position->_y))
							continue;
						switch (path._open.get_bit(position->_x, position->_y)) {
						case false:
							path._heap.push(new jump_point_search::path::node(*position, static_cast<jump_point_search::direction>(dir), current, path._destination));
							path._open.set_bit(position->_x, position->_y, true);
							break;
						case true:
							path._heap.shift_up(*position, current);
							break;
						}
					}
					path._parent.emplace_back(current);
				}
				else {
					_state = test::state::goal;
					break;
				}
			}

			//{
			//	QueryPerformanceCounter(&_start);
			//	path.search();
			//	QueryPerformanceCounter(&_end);
			//	_sum += _end.QuadPart - _start.QuadPart;
			//	++_count;
			//	auto result = (static_cast<double>(_sum) / _count) / static_cast<double>(_frequency.QuadPart) * 1e3;
			//	_state = test::state::goal;
			//}
			break;
		case state::goal:
			if (wait > _wait)
				_state = test::state::start;
			wait += 1;
			break;
		}
	}
public:
	type _type = type::shot;

	state _state;
	float _wall_probability = 0.48f;
	size_type _repeat = 0;
	size_type _skip = 1;
	size_type _wait = 20;

	long long _sum = 0;
	long long _count = 0;
	LARGE_INTEGER _frequency;
	LARGE_INTEGER _start, _end;
};