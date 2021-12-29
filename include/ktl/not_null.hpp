// KTL single-header library
// Requirements: C++20

#pragma once
#include <cassert>
#include <concepts>
#include <utility>

namespace ktl {
template <typename T>
concept comparable_to_nullptr = requires(T const& t) {
	{ t != nullptr } -> std::convertible_to<bool>;
};

///
/// \brief Wrapper for raw / smart pointers that is restricted from being null
///
template <comparable_to_nullptr T>
class not_null {
  public:
	using type = T;

	///
	/// \brief Generic implicit constructor
	///
	template <typename U>
		requires std::is_convertible_v<U, T>
	constexpr not_null(U&& rhs) noexcept : m_ptr(std::forward<U>(rhs)) { assert(m_ptr != nullptr); }
	///
	/// \brief Deleted constructor(s)
	///
	constexpr not_null(std::nullptr_t) = delete;
	constexpr not_null& operator=(std::nullptr_t) = delete;
	///
	/// \brief Obtain const lvalue reference to pointer from non-rvalue this
	///
	[[nodiscard]] constexpr T const& get() const& noexcept { return m_ptr; }
	///
	/// \brief Move pointer from rvalue this
	///
	[[nodiscard]] constexpr T get() && noexcept { return std::move(m_ptr); }
	[[nodiscard]] constexpr operator T const&() const noexcept { return get(); }
	constexpr decltype(auto) operator*() const noexcept { return *get(); }
	constexpr decltype(auto) operator->() const noexcept { return get(); }

  private:
	T m_ptr;
};
} // namespace ktl
