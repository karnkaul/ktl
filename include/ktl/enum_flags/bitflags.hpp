// KTL header-only library
// Requirements: C++17

#pragma once
#include <type_traits>

namespace ktl::flags {
///
/// \brief Test if all bits in mask are set in flags
///
template <typename T>
[[nodiscard]] constexpr bool all(T const flags, T const mask) noexcept {
	static_assert(std::is_integral_v<T>);
	return (flags & mask) == mask;
}
///
/// \brief Test if any bits in mask are set in flags
///
template <typename T>
[[nodiscard]] constexpr bool any(T const flags, T const mask) noexcept {
	static_assert(std::is_integral_v<T>);
	return (flags & mask) != T{};
}
///
/// \brief Update flags: unset active bits in unset, and set active bits in set
///
template <typename T>
[[nodiscard]] constexpr T update(T const flags, T const set, T const unset) noexcept {
	static_assert(std::is_integral_v<T>);
	T ret = flags & static_cast<T>(~unset);
	return ret | static_cast<T>(set);
}
///
/// \brief Count number of bits set in flags
///
template <typename T>
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
