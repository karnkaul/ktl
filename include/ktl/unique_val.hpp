// KTL header-only library
// Requirements: C++20

#pragma once
#include <concepts>
#include <utility>

namespace ktl {
///
/// \brief Models a unique value: resets to default on move
///
template <std::default_initializable Type>
struct unique_val {
	constexpr unique_val(Type value = {}) noexcept : value(std::move(value)) {}

	constexpr unique_val(unique_val&& rhs) noexcept : unique_val() { swap(rhs); }
	constexpr unique_val(unique_val const& rhs) noexcept : unique_val(rhs.value) {}
	constexpr unique_val& operator=(unique_val rhs) noexcept { return (swap(rhs), *this); }

	constexpr void swap(unique_val& rhs) noexcept {
		using std::swap;
		swap(value, rhs.value);
	}

	constexpr operator Type const&() const { return value; }

	Type value{};
};
} // namespace ktl
