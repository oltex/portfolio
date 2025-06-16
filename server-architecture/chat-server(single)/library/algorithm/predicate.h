#pragma once
#include <compare>

namespace algorithm::predicate {
	template<typename type>
	inline static auto less(type const& source, type const& destination) noexcept {
		return source <=> destination;
	}
	template<typename type>
	inline static auto greater(type const& source, type const& destination) noexcept {
		return destination <=> source;
	}
}