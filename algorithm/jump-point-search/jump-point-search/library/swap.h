#pragma once
#include <utility>

namespace library {
	template<typename type>
	inline void swap(type& left, type& right) noexcept {
		type temp = std::move(left);
		left = std::move(right);
		right = std::move(temp);
	}
}
