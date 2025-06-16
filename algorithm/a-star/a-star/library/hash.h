#pragma once

namespace library {
	template<typename type, typename size_type = unsigned int>
	inline constexpr auto hash(type const& key) noexcept -> size_type {
		constexpr size_type _FNV_offset_basis = sizeof(size_type) == 4 ? 2166136261U : 14695981039346656037ULL;
		constexpr size_type _FNV_prime = sizeof(size_type) == 4 ? 16777619U : 1099511628211ULL;

		size_type value = _FNV_offset_basis;
		unsigned char const* const byte = reinterpret_cast<unsigned char const*>(&key);
		for (size_type index = 0; index < sizeof(type); ++index) {
			value ^= static_cast<size_type>(byte[index]);
			value *= _FNV_prime;
		}
		return value;
	}
}