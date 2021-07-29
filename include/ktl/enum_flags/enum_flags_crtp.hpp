// KTL header-only library
// Requirements: C++17

#pragma once
#include <cstdint>
#include <type_traits>

namespace ktl::detail {
///
/// \brief CRTP base type for concrete flags
/// Requirements:
///  - [explicit] operator Ty() const noexcept
///  - EF& update(T, U) noexcept
///  - Ty& get_ty() noexcept
///
template <typename EF, typename Ty>
struct t_enum_flags_ {
	using type = EF;
	using value_type = Ty;

	///
	/// \brief Build an instance by setting inputs
	///
	template <typename... T>
	static constexpr EF make(T... t) noexcept;
	///
	/// \brief Set inputs
	///
	template <typename... T>
	constexpr EF& set(T... t) noexcept;
	///
	/// \brief Remove inputs
	///
	template <typename... T>
	constexpr EF& reset(T... t) noexcept;
	///
	/// \brief Assign value to mask bits
	///
	template <typename T>
	constexpr EF& assign(T mask, bool gvalue) noexcept;

	///
	/// \brief Test if any bits are set
	///
	constexpr bool any() const noexcept { return to_ty() != Ty{}; }
	///
	/// \brief Test if any bits in mask are set
	///
	template <typename T>
	constexpr bool any(T mask) const noexcept;
	///
	/// \brief Test if all bits in mask are set
	///
	template <typename T>
	constexpr bool all(T mask) const noexcept;
	///
	/// \brief Obtain number of set bits
	///
	constexpr std::size_t count() const noexcept;

	///
	/// \brief Compare two t_enum_flags_
	///
	friend constexpr bool operator==(EF a, EF b) noexcept { return a.to_ty() == b.to_ty(); }
	///
	/// \brief Compare two t_enum_flags_
	///
	friend constexpr bool operator!=(EF a, EF b) noexcept { return !(a == b); }

	///
	/// \brief Perform bitwise OR / add flags
	///
	template <typename T>
	constexpr EF& operator|=(T mask) noexcept;
	///
	/// \brief Perform bitwise AND / multiply flags
	///
	template <typename T>
	constexpr EF& operator&=(T mask) noexcept;
	///
	/// \brief Perform bitwise XOR / exclusively add flags (add mod 2)
	///
	template <typename T>
	constexpr EF& operator^=(T mask) noexcept;

	///
	/// \brief Perform bitwise OR / add flags
	///
	friend constexpr EF operator|(EF lhs, EF rhs) noexcept { return lhs |= rhs; }
	///
	/// \brief Perform bitwise AND / multiply flags
	///
	friend constexpr EF operator&(EF lhs, EF rhs) noexcept { return lhs &= rhs; }
	///
	/// \brief Perform bitwise XOR / exclusively add flags (add mod 2)
	///
	friend constexpr EF operator^(EF lhs, EF rhs) noexcept { return lhs ^= rhs; }

  private:
	constexpr EF& to_ef() noexcept { return static_cast<EF&>(*this); }
	constexpr EF const& to_ef() const noexcept { return static_cast<EF const&>(*this); }
	constexpr Ty to_ty() const noexcept { return static_cast<Ty>(to_ef()); }
	constexpr Ty& get_ty() noexcept { return to_ef().get_ty(); }
};

// impl

template <typename EF, typename Ty>
template <typename... T>
constexpr EF t_enum_flags_<EF, Ty>::make(T... t) noexcept {
	EF ret{};
	(ret.update(t), ...);
	return ret;
}
template <typename EF, typename Ty>
template <typename... T>
constexpr EF& t_enum_flags_<EF, Ty>::set(T... t) noexcept {
	auto& ret = to_ef();
	(ret.update(t), ...);
	return ret;
}
template <typename EF, typename Ty>
template <typename... T>
constexpr EF& t_enum_flags_<EF, Ty>::reset(T... t) noexcept {
	auto& ret = to_ef();
	(ret.update({}, t), ...);
	return to_ef();
}
template <typename EF, typename Ty>
template <typename T>
constexpr EF& t_enum_flags_<EF, Ty>::assign(T mask, bool get_ty) noexcept {
	if (get_ty) {
		set(mask);
	} else {
		reset(mask);
	}
	return to_ef();
}
template <typename EF, typename Ty>
template <typename T>
constexpr bool t_enum_flags_<EF, Ty>::any(T mask) const noexcept {
	return (to_ty() & make(mask).to_ty()) != Ty{};
}
template <typename EF, typename Ty>
template <typename T>
constexpr bool t_enum_flags_<EF, Ty>::all(T mask) const noexcept {
	EF const& t = make(mask);
	return (to_ty() & t.to_ty()) == t.to_ty();
}
template <typename EF, typename Ty>
constexpr std::size_t t_enum_flags_<EF, Ty>::count() const noexcept {
	std::size_t ret{};
	Ty bit = static_cast<Ty>(1);
	for (std::size_t i = 0; i < sizeof(Ty) * 8; ++i) {
		if ((to_ty() & bit) != 0) { ++ret; }
		bit <<= 1;
	}
	return ret;
}
template <typename EF, typename Ty>
template <typename T>
constexpr EF& t_enum_flags_<EF, Ty>::operator|=(T mask) noexcept {
	get_ty() |= make(mask).to_ty();
	return to_ef();
}
template <typename EF, typename Ty>
template <typename T>
constexpr EF& t_enum_flags_<EF, Ty>::operator&=(T mask) noexcept {
	get_ty() &= make(mask).to_ty();
	return to_ef();
}
template <typename EF, typename Ty>
template <typename T>
constexpr EF& t_enum_flags_<EF, Ty>::operator^=(T mask) noexcept {
	get_ty() ^= make(mask).to_ty();
	return to_ef();
}
} // namespace ktl::detail
