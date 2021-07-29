// KTL header-only library
// Requirements: C++17

#pragma once
#include "enum_flags_crtp.hpp"
#include "enum_traits.hpp"

namespace ktl {
///
/// \brief Wrapper around an integral type used as bit flags
///
template <typename Enum, typename Ty = std::uint32_t, typename Tr = enum_trait_linear>
class enum_flags : public detail::t_enum_flags_<enum_flags<Enum, Ty, Tr>, Ty> {
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum");
	static_assert(std::is_integral_v<Ty>, "Ty must be integral");
	static_assert(std::is_same_v<Tr, enum_trait_linear> || std::is_same_v<Tr, enum_trait_pot>, "Invalid enum trait");

  public:
	using type = Enum;
	using storage_t = Ty;
	static constexpr bool is_linear_v = std::is_same_v<Tr, enum_trait_linear>;

	///
	/// \brief Default constructor
	///
	constexpr enum_flags() = default;
	///
	/// \brief Set flag e
	///
	constexpr enum_flags(Enum e) noexcept;
	///
	/// \brief Set inputs
	///
	template <typename... T>
	constexpr enum_flags(T... t) noexcept;
	///
	/// \brief Conversion operator
	///
	constexpr explicit operator Ty() const noexcept { return m_bits; }

	///
	/// \brief Add set bits and remove unset bits
	///
	constexpr enum_flags& update(enum_flags set, enum_flags reset = {}) noexcept;
	///
	/// \brief Test for flag
	///
	constexpr bool test(Enum flag) const noexcept { return this->all(flag); }
	///
	/// \brief Test for flag
	///
	constexpr bool operator[](Enum flag) const noexcept { return this->test(flag); }

  private:
	constexpr Ty& get_ty() noexcept { return m_bits; }

	Ty m_bits{};

	template <typename T, typename U>
	friend struct detail::t_enum_flags_;
};

// impl

template <typename Enum, typename Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>::enum_flags(Enum e) noexcept {
	if constexpr (is_linear_v) {
		m_bits |= (1 << static_cast<Ty>(e));
	} else {
		m_bits |= static_cast<Ty>(e);
	}
}
template <typename Enum, typename Ty, typename Tr>
template <typename... T>
constexpr enum_flags<Enum, Ty, Tr>::enum_flags(T... t) noexcept {
	this->set(t...);
}
template <typename Enum, typename Ty, typename Tr>
constexpr enum_flags<Enum, Ty, Tr>& enum_flags<Enum, Ty, Tr>::update(enum_flags set, enum_flags unset) noexcept {
	m_bits |= set.m_bits;
	m_bits &= ~unset.m_bits;
	return *this;
}
} // namespace ktl
