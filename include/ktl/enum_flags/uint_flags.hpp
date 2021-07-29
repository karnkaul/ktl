// KTL header-only library
// Requirements: C++17

#pragma once
#include "enum_flags_crtp.hpp"

namespace ktl {
///
/// \brief Trivial wrapper for unsigned int as bit flags (union friendly)
///
template <typename Ty = std::uint32_t>
struct uint_flags : detail::t_enum_flags_<uint_flags<Ty>, Ty> {
	static_assert(std::is_unsigned_v<Ty>, "Ty must be unsigned");

	using type = Ty;
	using value_type = Ty;

	///
	/// \brief Trivial storage (default initialized)
	///
	Ty bits;

	///
	/// \brief Conversion operator
	///
	constexpr explicit operator Ty() const noexcept { return bits; }

	///
	/// \brief Test if all bits in t are set
	///
	template <typename T>
	constexpr bool operator[](T t) const noexcept {
		return this->all(t);
	}
	///
	/// \brief Add set bits and remove unset bits
	///
	template <typename T, typename U = T>
	constexpr uint_flags<Ty>& update(T set, U unset = {}) noexcept {
		bits |= static_cast<Ty>(set);
		bits &= ~static_cast<Ty>(unset);
		return *this;
	}

  private:
	constexpr Ty& get_ty() noexcept { return bits; }

	template <typename T, typename U>
	friend struct ktl::detail::t_enum_flags_;
};
} // namespace ktl
