#pragma once
#include "pair.h"
#include "hash.h"
#include "list.h"
#include "vector.h"

namespace library {
	template<typename key_type, typename type, auto _hash = hash<key_type>>
	class unordered_map final {
	private:
		using size_type = unsigned int;
		using pair = pair<key_type, type>;
	public:
		using iterator = typename list<pair>::iterator;
	public:
		inline explicit unordered_map(void) noexcept {
			rehash(_count);
		}
		inline explicit unordered_map(unordered_map const& rhs) noexcept;
		inline explicit unordered_map(unordered_map&& rhs) noexcept;
		inline auto operator=(unordered_map const& rhs) noexcept -> unordered_map&;
		inline auto operator=(unordered_map&& rhs) noexcept -> unordered_map&;
		inline ~unordered_map(void) noexcept = default;
	public:
		template<typename universal>
		inline void insert(key_type const& key, universal&& value) const noexcept {
			emplace(key, std::forward<universal>(value));
		}
		template<typename... argument>
		inline auto emplace(key_type const& key, argument&&... arg) noexcept -> iterator {
			auto iter = find(key);
			if (iter != _list.end())
				return iter;

			size_type index = bucket(key);
			auto& first = _vector[index << 1];
			auto& last = _vector[(index << 1) + 1];

			auto result = _list.emplace(first, key, std::forward<argument>(arg)...);
			if (first == _list.end())
				last = result;
			first = result;

			if (1.f <= load_factor())
				rehash(_count < 512 ? _count * 8 : _count + 1);
			return result;
		}
		inline void erase(key_type const& key) noexcept {
			auto iter = find(key);
			if (iter != _list.end())
				erase(iter);
		}
		inline void erase(iterator const& iter) noexcept {
			size_type index = bucket((*iter)._first);

			auto& first = _vector[index << 1];
			auto& last = _vector[(index << 1) + 1];

			if (first == last)
				first = last = _list.end();
			else if (first == iter)
				++first;
			else if (last == iter)
				--last;
			_list.erase(iter);
		}
		inline auto operator[](key_type const& key) noexcept -> type& {
			return (*emplace(key))._second;
		}
	public:
		inline auto begin(void) const noexcept -> typename iterator {
			return _list.begin();
		}
		inline auto begin(size_type const index) const noexcept -> iterator {
			return _vector[index << 1];
		}
		inline auto end(void) const noexcept -> typename iterator {
			return _list.end();
		}
		inline auto end(size_type const index) const noexcept -> iterator {
			auto iter = _vector[(index << 1) + 1];
			if (_list.end() != iter)
				iter++;
			return iter;
		}
	public:
		inline auto size(void) const noexcept -> size_type {
			return _list.size();
		}
		//not implemented (bucket size)
		inline auto size(size_type const index) const noexcept -> size_type;
		inline bool empty(void) const noexcept {
			return  _list.empty();
		}
		inline auto count(void) const noexcept -> size_type {
			return _count;
		}
		inline auto bucket(key_type const& key) const noexcept -> size_type {
			return _hash(key) % _count;
		}
		inline void clear(void) noexcept {
			_vector.assign(_count << 1, _list.end());
			_list.clear();
		}
		inline auto find(key_type const& key) const noexcept -> iterator {
			size_type index = bucket(key);

			auto first = begin(index);
			auto last = end(index);
			auto end = _list.end();

			if (first != end) {
				for (auto iter = first;; ++iter) {
					if ((*iter)._first == key)
						return iter;
					if (iter == last)
						break;
				}
			}
			return end;
		}
	public:
		inline auto load_factor(void) const noexcept -> float {
			return static_cast<float>(_list.size()) / _count;
		}
		inline void rehash(size_type const count) noexcept {
			unsigned long bit;
			_BitScanReverse64(&bit, ((count - 1) | 1));
			_count = static_cast<size_type>(1) << (1 + bit);

			auto iter = _list.begin();
			auto end = _list.end();
			_vector.assign(_count << 1, end);

			for (auto next = iter; iter != end; iter = next) {
				++next;
				auto index = bucket((*iter)._first);

				auto& first = _vector[index << 1];
				auto& last = _vector[(index << 1) + 1];

				if (first == end)
					last = iter;
				else
					_list.splice(first, iter, next);
				first = iter;
			}
		}
	private:
		vector<iterator> _vector;
		list<pair> _list;
		size_type _count = 8;
	};
}