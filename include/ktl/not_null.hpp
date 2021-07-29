// KTL single-header library
// Requirements: C++17

#pragma once
#include <cassert>
#include <type_traits>
#include <utility>

namespace ktl {
namespace detail {
template <typename T, typename = void>
struct is_comparable_to_nullptr : std::false_type {};
template <typename T>
struct is_comparable_to_nullptr<T, std::enable_if_t<std::is_convertible_v<decltype(std::declval<T>() != nullptr), bool>>> : std::true_type {};
template <typename T>
constexpr bool is_comparable_to_nullptr_v = is_comparable_to_nullptr<T>::value;
} // namespace detail

///
/// \brief Wrapper for raw / smart pointers that is restricted from being null
///
template <typename T>
class not_null {
	static_assert(detail::is_comparable_to_nullptr_v<T>, "Cannot compare T to nullptr");

  public:
	using type = T;

	///
	/// \brief Generic implicit constructor
	///
	template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
	constexpr not_null(U&& rhs) noexcept : m_ptr(std::forward<U>(rhs)) {
		assert(m_ptr != nullptr);
	}
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
	[[nodiscard]] constexpr operator T const &() const noexcept { return get(); }
	constexpr decltype(auto) operator*() const noexcept { return *get(); }
	constexpr decltype(auto) operator->() const noexcept { return get(); }

	friend constexpr bool operator==(not_null<T> const& lhs, not_null<T> const& rhs) noexcept { return lhs.get() == rhs.get(); }

  private:
	T m_ptr;
};
} // namespace ktl
