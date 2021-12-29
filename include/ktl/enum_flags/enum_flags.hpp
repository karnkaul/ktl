// KTL header-only library
// Requirements: C++20

#pragma once
#include <cstdint>
#include "bitflags.hpp"
#include "enum_traits.hpp"

namespace ktl {
///
/// \brief Wrapper around an integral type used as bit flags, constrained to use with a particular enum
/// \param Enum enum to associate bit flags with
/// \param Ty underlying type of bit flags (u32 by default)
/// \param Tr trait specifying whether enum is linear (default) or power-of-two
///
template <typename Enum, std::integral Ty = std::uint32_t, typename Tr = enum_trait_linear>
class enum_flags {
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum");
	static_assert(std::is_same_v<Tr, enum_trait_linear> || std::is_same_v<Tr, enum_trait_pot>, "Invalid enum trait");

  public:
	using type = Enum;
	using storage_t = Ty;
	static constexpr bool is_linear_v = std::is_same_v<Tr, enum_trait_linear>;

	template <typename... T>
	static constexpr bool valid_flag_v = (... && (std::is_same_v<T, enum_flags> || std::is_same_v<T, Enum>));

	constexpr enum_flags() = default;
	///
	/// \brief Set flag e
	///
	constexpr enum_flags(Enum e) noexcept;
	///
	/// \brief Set flags (T must be Enum)
	///
	template <typename... T>
		requires(valid_flag_v<T...>)
	constexpr enum_flags(T const... t) noexcept { set(t...); }
	///
	/// \brief Obtain underlying bits
	///
	[[nodiscard]] constexpr Ty bits() const noexcept { return m_bits; }
	///
	/// \brief Conversion operator
	///
	constexpr explicit operator Ty() const noexcept { return bits(); }

	///
	/// \brief Set flags (T must be Enum)
	///
	template <typename... T>
		requires(valid_flag_v<T...>)
	constexpr enum_flags& set(T const... t) noexcept { return ((update(enum_flags(t)), ...), *this); }
	///
	/// \brief Reset flags (T must be Enum)
	///
	template <typename... T>
		requires(valid_flag_v<T...>)
	constexpr enum_flags& reset(T const... t) noexcept { return ((update({}, enum_flags(t)), ...), *this); }
	///
	/// \brief Flip flags (T must be Enum)
	///
	template <typename... T>
		requires(valid_flag_v<T...>)
	constexpr enum_flags& flip(T const... t) noexcept { return ((do_flip(t), ...), *this); }
	///
	/// \brief Assign value to mask bits
	///
	constexpr enum_flags& assign(enum_flags mask, bool value) noexcept;
	///
	/// \brief Add set bits and remove unset bits
	///
	constexpr enum_flags& update(enum_flags set, enum_flags reset = {}) noexcept;

	///
	/// \brief Test if any bits are set
	///
	[[nodiscard]] constexpr bool any() const noexcept { return m_bits != Ty{}; }
	///
	/// \brief Test for flag
	///
	[[nodiscard]] constexpr bool test(Enum const flag) const noexcept { return all(flag); }
	///
	/// \brief Test for flag
	///
	[[nodiscard]] constexpr bool operator[](Enum const flag) const noexcept { return test(flag); }
	///
	/// \brief Test if any bits in mask are set
	///
	[[nodiscard]] constexpr bool any(enum_flags const mask) const noexcept { return flags::any(m_bits, mask.m_bits); }
	///
	/// \brief Test if all bits in mask are set
	///
	[[nodiscard]] constexpr bool all(enum_flags const mask) const noexcept { return flags::all(m_bits, mask.m_bits); }
	///
	/// \brief Obtain number of set bits
	///
	[[nodiscard]] constexpr std::size_t count() const noexcept { return flags::count(m_bits); }

	///
	/// \brief Comparison operator
	///
	[[nodiscard]] constexpr bool operator==(enum_flags const& lhs) const noexcept = default;

	///
	/// \brief Perform bitwise OR / add flags
	///
	constexpr enum_flags& operator|=(enum_flags mask) noexcept;
	///
	/// \brief Perform bitwise AND / multiply flags
	///
	constexpr enum_flags& operator&=(enum_flags mask) noexcept;
	///
	/// \brief Perform bitwise XOR / exclusively add flags (add mod 2)
	///
	constexpr enum_flags& operator^=(enum_flags mask) noexcept;

	///
	/// \brief Perform bitwise OR / add flags
	///
	[[nodiscard]] friend constexpr enum_flags operator|(enum_flags const lhs, enum_flags const rhs) noexcept { return enum_flags(lhs) |= rhs; }
	///
	/// \brief Perform bitwise AND / multiply flags
	///
	[[nodiscard]] friend constexpr enum_flags operator&(enum_flags const lhs, enum_flags const rhs) noexcept { return enum_flags(lhs) &= rhs; }
	///
	/// \brief Perform bitwise XOR / exclusively add flags (add mod 2)
	///
	[[nodiscard]] friend constexpr enum_flags operator^(enum_flags const lhs, enum_flags const rhs) noexcept { return enum_flags(lhs) ^= rhs; }

  private:
	template <typename T>
	constexpr void do_flip(T t) noexcept;

	Ty m_bits{};
};

// impl

template <typename Enum, std::integral Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>::enum_flags(Enum const e) noexcept {
	if constexpr (is_linear_v) {
		m_bits |= (1 << static_cast<Ty>(e));
	} else {
		m_bits |= static_cast<Ty>(e);
	}
}
template <typename Enum, std::integral Ty, typename Tr>
template <typename T>
constexpr void enum_flags<Enum, Ty, Tr>::do_flip(T const t) noexcept {
	if (test(t)) {
		reset(t);
	} else {
		set(t);
	}
}
template <typename Enum, std::integral Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>& enum_flags<Enum, Ty, Tr>::assign(enum_flags const mask, bool const value) noexcept {
	if (value) {
		set(mask);
	} else {
		reset(mask);
	}
	return *this;
}
template <typename Enum, std::integral Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>& enum_flags<Enum, Ty, Tr>::update(enum_flags const set, enum_flags const unset) noexcept {
	m_bits = flags::update(m_bits, set.m_bits, unset.m_bits);
	return *this;
}
template <typename Enum, std::integral Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>& enum_flags<Enum, Ty, Tr>::operator|=(enum_flags const mask) noexcept {
	m_bits |= mask.m_bits;
	return *this;
}
template <typename Enum, std::integral Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>& enum_flags<Enum, Ty, Tr>::operator&=(enum_flags const mask) noexcept {
	m_bits &= mask.m_bits;
	return *this;
}
template <typename Enum, std::integral Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>& enum_flags<Enum, Ty, Tr>::operator^=(enum_flags const mask) noexcept {
	m_bits ^= mask.m_bits;
	return *this;
}
} // namespace ktl
