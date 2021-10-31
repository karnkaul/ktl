// KTL header-only library
// Requirements: C++17

#pragma once
#include <cstdint>
#include "bitflags.hpp"

namespace ktl {
///
/// \brief Trivial bit flags wrapper
/// \param Ty underlying type of bit flags (u32 by default)
///
template <typename Ty = std::uint32_t>
struct uint_flags {
	using value_type = Ty;

	Ty value;

	///
	/// \brief Build an instance by setting inputs
	///
	template <typename... T>
	[[nodiscard]] static constexpr uint_flags make(T const... t) noexcept;
	///
	/// \brief Conversion operator
	///
	constexpr explicit operator Ty() const noexcept { return value; }
	///
	/// \brief Set inputs
	///
	template <typename... T>
	constexpr uint_flags& set(T const... t) noexcept;
	///
	/// \brief Reset inputs
	///
	template <typename... T>
	constexpr uint_flags& reset(T const... t) noexcept;
	///
	/// \brief Flip inputs
	///
	template <typename... T>
	constexpr uint_flags& flip(T const... t) noexcept;
	///
	/// \brief Assign value to mask bits
	///
	template <typename T>
	constexpr uint_flags& assign(T mask, bool value) noexcept;
	///
	/// \brief Set and unset inputs
	///
	template <typename T, typename U = T>
	constexpr uint_flags<Ty>& update(T set, U unset = {}) noexcept;

	///
	/// \brief Test if any bits are set
	///
	[[nodiscard]] constexpr bool any() const noexcept { return value != Ty{}; }
	///
	/// \brief Test if any bits in t are set
	///
	template <typename T>
	[[nodiscard]] constexpr bool test(T t) const noexcept;
	///
	/// \brief Test if any bits in t are set
	///
	template <typename T>
	[[nodiscard]] constexpr bool operator[](T t) const noexcept;
	///
	/// \brief Test if any bits in mask are set
	///
	template <typename T>
	[[nodiscard]] constexpr bool any(T mask) const noexcept;
	///
	/// \brief Test if all bits in mask are set
	///
	template <typename T>
	[[nodiscard]] constexpr bool all(T mask) const noexcept;
	///
	/// \brief Obtain number of set bits
	///
	[[nodiscard]] constexpr std::size_t count() const noexcept { return flags::count(value); }

	///
	/// \brief Perform bitwise OR / add flags
	///
	template <typename T>
	constexpr uint_flags& operator|=(T mask) noexcept;
	///
	/// \brief Perform bitwise AND / multiply flags
	///
	template <typename T>
	constexpr uint_flags& operator&=(T mask) noexcept;
	///
	/// \brief Perform bitwise XOR / exclusively add flags (add mod 2)
	///
	template <typename T>
	constexpr uint_flags& operator^=(T mask) noexcept;

	///
	/// \brief Perform bitwise OR / add flags
	///
	friend constexpr uint_flags operator|(uint_flags const lhs, uint_flags const rhs) noexcept { return uint_flags(lhs) |= rhs; }
	///
	/// \brief Perform bitwise AND / multiply flags
	///
	friend constexpr uint_flags operator&(uint_flags const lhs, uint_flags const rhs) noexcept { return uint_flags(lhs) &= rhs; }
	///
	/// \brief Perform bitwise XOR / exclusively add flags (add mod 2)
	///
	friend constexpr uint_flags operator^(uint_flags const lhs, uint_flags const rhs) noexcept { return uint_flags(lhs) ^= rhs; }
};

// impl

template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty> uint_flags<Ty>::make(T const... t) noexcept {
	uint_flags ret{};
	(ret.update(t), ...);
	return ret;
}
template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty>& uint_flags<Ty>::set(T const... t) noexcept {
	(update(t), ...);
	return *this;
}
template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty>& uint_flags<Ty>::reset(T const... t) noexcept {
	(update({}, t), ...);
	return *this;
}
template <typename Ty>
template <typename... T>
constexpr uint_flags<Ty>& uint_flags<Ty>::flip(T const... t) noexcept {
	auto do_flip = [&](auto const t) {
		if (test(t)) {
			reset(t);
		} else {
			set(t);
		}
	};
	(do_flip(t), ...);
	return *this;
}
template <typename Ty>
template <typename T>
constexpr uint_flags<Ty>& uint_flags<Ty>::assign(T const mask, bool const set) noexcept {
	if (set) {
		set(mask);
	} else {
		reset(mask);
	}
	return *this;
}
template <typename Ty>
template <typename T, typename U>
constexpr uint_flags<Ty>& uint_flags<Ty>::update(T const set, U const unset) noexcept {
	value = flags::update(value, static_cast<Ty>(set), static_cast<Ty>(unset));
	return *this;
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::test(T const t) const noexcept {
	return all(t);
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::operator[](T const t) const noexcept {
	return test(t);
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::any(T const mask) const noexcept {
	return flags::any(value, static_cast<Ty>(mask));
}
template <typename Ty>
template <typename T>
constexpr bool uint_flags<Ty>::all(T const mask) const noexcept {
	return flags::all(value, static_cast<Ty>(mask));
}
template <typename Ty>
template <typename T>
constexpr uint_flags<Ty>& uint_flags<Ty>::operator|=(T const mask) noexcept {
	static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
	value |= static_cast<Ty>(mask);
	return *this;
}
template <typename Ty>
template <typename T>
constexpr uint_flags<Ty>& uint_flags<Ty>::operator&=(T const mask) noexcept {
	static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
	value &= static_cast<Ty>(mask);
	return *this;
}
template <typename Ty>
template <typename T>
constexpr uint_flags<Ty>& uint_flags<Ty>::operator^=(T const mask) noexcept {
	static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
	value ^= static_cast<Ty>(mask);
	return *this;
}
} // namespace ktl
