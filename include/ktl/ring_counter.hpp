// KTL header-only library
// Requirements: C++20

#pragma once
#include <type_traits>
#include <utility>

namespace ktl {
///
/// \brief Fixed-size rotating integral counter. Supports increment/decrement.
///
template <typename T>
	requires std::is_integral_v<T>
class ring_counter {
  public:
	using type = T;

	constexpr ring_counter() = default;
	explicit constexpr ring_counter(T const size, T const value = {}) noexcept : m_value(value), m_size(size) {}

	constexpr ring_counter& operator=(T const value) noexcept { return (m_value = value, *this); }

	constexpr T get() const noexcept { return m_value; }
	constexpr operator T() const noexcept { return m_value; }
	constexpr ring_counter& operator++() noexcept { return *this += 1U; }
	constexpr ring_counter operator++(int) noexcept {
		auto ret = *this;
		++*this;
		return ret;
	}
	constexpr ring_counter& operator--() noexcept { return *this -= 1U; }
	constexpr ring_counter operator--(int) noexcept {
		auto ret = *this;
		--*this;
		return ret;
	}
	constexpr ring_counter& operator+=(T const rhs) noexcept { return (m_value = (m_value + rhs) % m_size, *this); }
	constexpr ring_counter& operator-=(T const rhs) noexcept { return (m_value = (m_size + m_value - rhs) % m_size, *this); }

	friend constexpr ring_counter operator+(ring_counter const& a, T const b) noexcept {
		auto ret = a;
		return ret += b;
	}
	friend constexpr ring_counter operator-(ring_counter const& a, T const b) noexcept {
		auto ret = a;
		return ret -= b;
	}

  private:
	T m_value{};
	T m_size{};
};

using ring_index = ring_counter<std::size_t>;
} // namespace ktl
