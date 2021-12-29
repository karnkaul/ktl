// KTL header-only library
// Requirements: C++20

#pragma once
#include <array>
#include <iterator>
#include <type_traits>
#include "enum_traits.hpp"

namespace ktl {
///
/// \brief Iterator for an enum value
///
template <typename Enum, typename Tr>
struct enum_iterator;

///
/// \brief (Stateless) container for values of an Enum and its given range
/// \param Enum enum to iterate over
/// \param Begin start value of enum (0 by default)
/// \param End one past the last valid value (Enum::eCOUNT_ by default)
/// \param Tr enum_trait_linear or enum_trait_pot (linear by default)
///
template <typename Enum, Enum Begin = static_cast<Enum>(0), Enum End = Enum::eCOUNT_, typename Tr = enum_trait_linear>
struct enumerate_enum {
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum");

	using value_type = Enum;
	using enum_traits = Tr;
	using const_iterator = enum_iterator<Enum, Tr>;
	using u_type = std::underlying_type_t<Enum>;

	///
	/// \brief Value indicating if enum is power of two
	///
	static constexpr bool is_pot_v = std::is_same_v<Tr, enum_trait_pot>;
	///
	/// \brief Obtain count of represented range
	///
	static constexpr std::size_t size() noexcept;

	static_assert(!is_pot_v || std::is_unsigned_v<u_type>, "Signed POT enums are not allowed");

	static constexpr const_iterator begin() noexcept { return const_iterator{Begin}; }
	static constexpr const_iterator end() noexcept { return const_iterator{End}; }

	///
	/// \brief Obtain all values of represented range in an array
	///
	static constexpr std::array<Enum, size()> values() noexcept {
		std::array<Enum, size()> ret;
		std::size_t i = 0;
		for (auto const e : enumerate_enum<Enum, Begin, End, Tr>()) { ret[i++] = e; }
		return ret;
	}
};

// impl

template <typename Enum, typename Tr>
struct enum_iterator {
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum");

	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = Enum;
	using u_type = std::underlying_type_t<Enum>;

	static constexpr bool is_pot_v = std::is_same_v<Tr, enum_trait_pot>;

	static_assert(!is_pot_v || std::is_unsigned_v<u_type>, "Signed POT enums are not allowed");

	Enum value{};

	constexpr value_type operator*() const noexcept { return value; }
	constexpr enum_iterator& operator++() noexcept {
		if constexpr (is_pot_v) {
			value = static_cast<Enum>(static_cast<u_type>(value) << 1);
		} else {
			value = static_cast<Enum>(static_cast<u_type>(value) + 1);
		}
		return *this;
		;
	}
	constexpr enum_iterator operator++(int) noexcept {
		auto ret = *this;
		++(*this);
		return ret;
	}
	constexpr enum_iterator& operator--() noexcept {
		if constexpr (is_pot_v) {
			value = static_cast<Enum>(static_cast<u_type>(value) >> 1);
		} else {
			value = static_cast<Enum>(static_cast<u_type>(value) - 1);
		}
		return *this;
	}
	constexpr enum_iterator operator--(int) noexcept {
		auto ret = *this;
		--(*this);
		return ret;
	}

	constexpr auto operator<=>(enum_iterator const&) const = default;
};

template <typename Enum, Enum Begin, Enum End, typename Tr>
constexpr std::size_t enumerate_enum<Enum, Begin, End, Tr>::size() noexcept {
	if constexpr (std::is_same_v<Tr, enum_trait_pot>) {
		std::size_t ret{};
		for (auto begin = Begin; begin != End; begin = static_cast<Enum>(static_cast<u_type>(begin) << 1)) { ++ret; }
		return ret;
	} else {
		return static_cast<std::size_t>(End) - static_cast<std::size_t>(Begin);
	}
}
} // namespace ktl
