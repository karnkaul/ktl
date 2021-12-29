// KTL header-only library
// Requirements: C++20

#pragma once
#include <concepts>

namespace ktl::flags {
///
/// \brief Test if all bits in mask are set in flags
///
template <std::integral T>
[[nodiscard]] constexpr bool all(T const flags, T const mask) noexcept {
	return (flags & mask) == mask;
}
///
/// \brief Test if any bits in mask are set in flags
///
template <std::integral T>
[[nodiscard]] constexpr bool any(T const flags, T const mask) noexcept {
	return (flags & mask) != T{};
}
///
/// \brief Update flags: unset active bits in unset, and set active bits in set
///
template <std::integral T>
[[nodiscard]] constexpr T update(T const flags, T const set, T const unset) noexcept {
	T ret = flags & static_cast<T>(~unset);
	return ret | static_cast<T>(set);
}
///
/// \brief Count number of bits set in flags
///
template <std::integral T>
[[nodiscard]] constexpr std::size_t count(T const flags) noexcept {
	std::size_t ret{};
	T bit = static_cast<T>(1);
	for (std::size_t i = 0; i < sizeof(T) * 8; ++i) {
		if ((flags & bit) != T{}) { ++ret; }
		bit <<= 1;
	}
	return ret;
}
} // namespace ktl::flags
