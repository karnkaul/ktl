// KTL header=only library
// Requirements: C++20

#pragma once
#include "either.hpp"

namespace ktl {
///
/// \brief Wrapper type for any T
///
template <typename T>
struct unexpected {
	using type = T;

	T payload;

	constexpr unexpected(T&& t) noexcept(std::is_nothrow_move_constructible_v<T>) : payload(std::move(t)) {}
	constexpr unexpected(T const& t) noexcept(std::is_nothrow_copy_constructible_v<T>) : payload(t) {}
	template <typename... Args>
	constexpr unexpected(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : payload(std::forward<Args>(args)...) {}
};

///
/// \brief Models an expected value an unexpected error
///
template <typename T, typename E>
class expected {
  public:
	template <typename Ty>
	using resolve_t = typename either<T, E>::template resolve_t<Ty>;
	///
	/// \brief Construct a value (default) or error
	///
	template <typename Ty = T>
	constexpr expected(Ty&& t = T{}) noexcept(std::is_nothrow_constructible_v<resolve_t<Ty>, Ty>);
	///
	/// \brief Explicitly construct an error
	///
	constexpr expected(unexpected<E> e) noexcept(std::is_nothrow_move_constructible_v<E>) : m_either(std::move(e.payload)), m_err(true) {}

	constexpr bool has_value() const noexcept { return !m_err; }
	constexpr bool has_error() const noexcept { return m_err; }
	///
	/// \brief Check if instance has value
	///
	constexpr explicit operator bool() const noexcept { return has_value(); }

	///
	/// \brief Obtain a const lvalue reference to the held value (throws if error held)
	///
	constexpr T const& value() const& noexcept(false);
	///
	/// \brief Obtain an lvalue reference to the held value (throws if error held)
	///
	constexpr T& value() & noexcept(false);
	///
	/// \brief Obtain an rvalue reference to the held value (throws if error held)
	///
	constexpr T&& value() && noexcept(false);

	///
	/// \brief Obtain a const lvalue reference to the value if held, else fallback
	///
	constexpr T const& value_or(T const& fallback = T{}) noexcept { return has_value() ? value() : fallback; }

	///
	/// \brief Obtain a const lvalue reference to the held error (asserts if value held)
	///
	constexpr E const& error() const& noexcept;
	///
	/// \brief Obtain an lvalue reference to the held error (asserts if value held)
	///
	constexpr E& error() & noexcept;
	///
	/// \brief Obtain an rvalue reference to the held error (asserts if value held)
	///
	constexpr E&& error() && noexcept;

	constexpr T const& operator*() const& { return value(); }
	constexpr T& operator*() & { return value(); }
	constexpr T&& operator*() && { return std::move(value()); }

	constexpr T const* operator->() const { return &value(); }
	constexpr T* operator->() { return &value(); }

  private:
	either<T, E> m_either;
	bool m_err = false;
};

///
/// \brief Models expected success or an unexpected error
///
template <typename E>
class expected<void, E> {
  public:
	///
	/// \brief Obtain a success value
	///
	static constexpr expected success() noexcept { return expected(); }
	///
	/// \brief Construct a (success) value
	///
	constexpr expected() = default;
	///
	/// \brief Construct an error value
	///
	constexpr expected(E e) noexcept(std::is_nothrow_move_constructible_v<E>) : m_either(std::move(e)) {}
	///
	/// \brief Explicitly construct an error
	///
	constexpr expected(unexpected<E> e) noexcept(std::is_nothrow_move_constructible_v<E>) : m_either(std::move(e.payload)) {}

	constexpr bool has_value() const noexcept { return m_either.template contains<dummy_t>(); }
	constexpr bool has_error() const noexcept { return m_either.template contains<E>(); }
	///
	/// \brief Check if instance represents success
	///
	constexpr explicit operator bool() const noexcept { return has_value(); }

	///
	/// \brief Obtain a const lvalue reference to the held error (asserts if not error)
	///
	constexpr E const& error() const& noexcept;
	///
	/// \brief Obtain an lvalue reference to the held error (asserts if not error)
	///
	constexpr E& error() & noexcept;
	///
	/// \brief Obtain an rvalue reference to the held error (asserts if not error)
	///
	constexpr E&& error() && noexcept;

  private:
	struct dummy_t {};
	either<dummy_t, E> m_either;
};

// impl

template <typename T, typename E>
template <typename Ty>
constexpr expected<T, E>::expected(Ty&& t) noexcept(std::is_nothrow_constructible_v<resolve_t<Ty>, Ty>)
	: m_either(std::forward<Ty>(t)), m_err(!m_either.template contains<T>()) {}

template <typename T, typename E>
constexpr T const& expected<T, E>::value() const& {
	if (m_err) { throw error(); }
	return m_either.template get<T>();
}

template <typename T, typename E>
constexpr T& expected<T, E>::value() & {
	if (m_err) { throw error(); }
	return m_either.template get<T>();
}

template <typename T, typename E>
constexpr T&& expected<T, E>::value() && {
	if (m_err) { throw error(); }
	return std::move(m_either).template get<T>();
}

template <typename T, typename E>
constexpr E const& expected<T, E>::error() const& noexcept {
	assert(m_err);
	return m_either.template get<E>();
}

template <typename T, typename E>
constexpr E& expected<T, E>::error() & noexcept {
	assert(m_err);
	return m_either.template get<E>();
}

template <typename T, typename E>
constexpr E&& expected<T, E>::error() && noexcept {
	assert(m_err);
	return std::move(m_either).template get<E>();
}

template <typename E>
constexpr E const& expected<void, E>::error() const& noexcept {
	assert(has_error());
	return m_either.template get<E>();
}

template <typename E>
constexpr E& expected<void, E>::error() & noexcept {
	assert(has_error());
	return m_either.template get<E>();
}

template <typename E>
constexpr E&& expected<void, E>::error() && noexcept {
	assert(has_error());
	return std::move(m_either).template get<E>();
}
} // namespace ktl
