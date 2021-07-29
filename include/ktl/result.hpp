// KTL header-only library
// Requirements: C++17

#pragma once
#include <cassert>
#include <optional>
#include <type_traits>
#include <variant>

namespace ktl {
namespace detail {
template <typename T, typename E>
struct result_storage_t;
} // namespace detail

///
/// \brief Models a result (T) or an error (E) value
/// Note: T cannot be void
/// Specializations:
/// 	- T, T : homogeneous result and error types
/// 	- T, void : result type only (like optional)
/// 	- bool, void : boolean result only (like bool)
///
template <typename T, typename E = void>
class result;

///
/// \brief Type alias for no result
///
constexpr auto null_result = nullptr;

///
/// \brief Models a result (T) or an error (E) value
/// Note: T cannot be void
///
template <typename T, typename E>
class result {
	static_assert(!std::is_same_v<T, void>, "T = void is not supported");

  public:
	using type = T;
	using err_t = E;

	///
	/// \brief Default constructor (failure)
	///
	constexpr result() = default;
	///
	/// \brief Constructor for result (success)
	///
	constexpr result(T&& t) : m_storage(std::move(t)) {}
	///
	/// \brief Constructor for result (success)
	///
	constexpr result(T const& t) : m_storage(t) {}
	///
	/// \brief Constructor for error (failure)
	///
	constexpr result(E&& e) : m_storage(std::move(e)) {}
	///
	/// \brief Constructor for error (failure)
	///
	constexpr result(E const& e) : m_storage(e) {}
	///
	/// \brief Constructor for implicit failure
	///
	constexpr result(std::nullptr_t) : result() {}

	constexpr explicit operator bool() const noexcept { return has_value(); }
	constexpr bool has_value() const noexcept { return m_storage.has_value(); }
	constexpr bool has_error() const noexcept { return !has_value(); }

	///
	/// \brief Obtain const lvalue ref to result from non-rvalue this
	///
	constexpr T const& value() const& { return m_storage.value(); }
	///
	/// \brief Move result from rvalue this
	///
	constexpr T value() const&& { return std::move(m_storage).value(); }
	constexpr T const& value_or(T const& fallback) const { return has_value() ? value() : fallback; }
	constexpr E const& error() const { return m_storage.error(); }

	constexpr T const& operator*() const { return value(); }
	constexpr T const* operator->() const { return &value(); }

  private:
	detail::result_storage_t<T, err_t> m_storage;
};

///
/// \brief Models a result or an error value (T)
/// Note: T cannot be void
///
template <typename T>
class result<T, T> {
	static_assert(!std::is_same_v<T, void>, "T = void is not supported");

  public:
	using type = T;
	using err_t = T;

	///
	/// \brief Default constructor (failure)
	///
	constexpr result() : m_storage(T{}), m_error(true) {}
	///
	/// \brief Constructor for implicit failure
	///
	constexpr result(std::nullptr_t) : result() {}

	constexpr void set_result(T&& t) { set(std::move(t), false); }
	constexpr void set_result(T const& t) { set(t, false); }
	constexpr void set_error(T&& e) { set(std::move(e), true); }
	constexpr void set_error(T const& e) { set(e, true); }

	constexpr explicit operator bool() const noexcept { return has_value(); }
	constexpr bool has_value() const noexcept { return !m_error; }
	constexpr bool has_error() const noexcept { return !has_value(); }

	///
	/// \brief Obtain const lvalue ref to result from non-rvalue this
	///
	constexpr T const& value() const& { return get<T const&>(m_storage, !m_error); }
	///
	/// \brief Move result from rvalue this
	///
	constexpr T value() && { return get<T>(std::move(m_storage), !m_error); }
	constexpr T const& value_or(T const& fallback) const { return has_value() ? value() : fallback; }
	constexpr T const& error() const { return get<T const&>(m_storage, m_error); }

	constexpr T const& operator*() const { return value(); }
	constexpr T const* operator->() const { return &value(); }

  private:
	template <typename U>
	constexpr void set(U&& u, bool error) {
		m_storage.emplace(std::forward<U>(u));
		m_error = error;
	}
	template <typename U, typename V>
	static constexpr U get(V&& storage, [[maybe_unused]] bool pred) {
		assert(pred);
		return std::forward<V>(storage).value();
	}

	detail::result_storage_t<type, void> m_storage;
	bool m_error = true;
};

///
/// \brief Models an optional result (T)
/// Note: T cannot be void
///
template <typename T>
class result<T, void> {
	static_assert(!std::is_same_v<T, void>, "T = void is not supported");

  public:
	using type = T;

	///
	/// \brief Default constructor (failure)
	///
	constexpr result() = default;
	///
	/// \brief Constructor for result (success)
	///
	constexpr result(T&& t) : m_storage(std::move(t)) {}
	///
	/// \brief Constructor for result (success)
	///
	constexpr result(T const& t) : m_storage(t) {}
	///
	/// \brief Constructor for implicit failure
	///
	constexpr result(std::nullptr_t) : result() {}

	constexpr explicit operator bool() const noexcept { return has_value(); }
	constexpr bool has_value() const noexcept { return m_storage.has_value(); }
	constexpr bool has_error() const noexcept { return !has_value(); }

	///
	/// \brief Obtain const lvalue ref to result from non-rvalue this
	///
	constexpr T const& value() const& { return m_storage.value(); }
	///
	/// \brief Move result from rvalue this
	///
	constexpr T value() && { return std::move(m_storage).value(); }
	///
	/// \brief Obtain result if success else fallback
	///
	constexpr T const& value_or(T const& fallback) const { return has_value() ? value() : fallback; }

	constexpr T const& operator*() const { return value(); }
	constexpr T const* operator->() const { return &value(); }

  private:
	detail::result_storage_t<T, void> m_storage;
};

namespace detail {
template <typename T, typename E>
struct result_storage_t {
	std::variant<T, E> val;

	constexpr result_storage_t() : val(E{}) {}
	constexpr result_storage_t(T&& t) : val(std::move(t)) {}
	constexpr result_storage_t(T const& t) : val(t) {}
	constexpr result_storage_t(E&& e) : val(std::move(e)) {}
	constexpr result_storage_t(E const& e) : val(e) {}
	constexpr bool has_value() const noexcept { return std::holds_alternative<T>(val); }
	constexpr T const& value() const& {
		assert(has_value());
		return std::get<T>(val);
	}
	constexpr T value() && {
		assert(has_value());
		return std::get<T>(std::move(val));
	}
	constexpr E const& error() const {
		assert(!has_value());
		return std::get<E>(val);
	}
};
template <typename T>
struct result_storage_t<T, void> {
	std::optional<T> val;

	constexpr result_storage_t() = default;
	constexpr result_storage_t(T&& t) : val(std::move(t)) {}
	constexpr result_storage_t(T const& t) : val(t) {}
	constexpr bool has_value() const noexcept { return val.has_value(); }
	constexpr T const& value() const& {
		assert(has_value());
		return *val;
	}
	constexpr T value() && {
		assert(has_value());
		return std::move(*val);
	}
};
template <>
struct result_storage_t<bool, void> {
	bool val;

	constexpr result_storage_t() : val(false) {}
	constexpr result_storage_t(bool val) : val(val) {}
	constexpr bool has_value() const noexcept { return val; }
	constexpr bool value() const { return val; }
};
} // namespace detail
} // namespace ktl
