// KTL single-header library
// Requirements: C++20

#pragma once
#include "fixed_any.hpp"
#include "koverloaded.hpp"

namespace ktl {
namespace detail {
template <typename... T>
struct largest_sizeof;
template <typename... Types>
static constexpr std::size_t largest_sizeof_v = largest_sizeof<Types...>::value;
template <typename Head, typename...>
struct head_of {
	using type = Head;
};
template <typename... Types>
using head_of_t = typename head_of<Types...>::type;
template <typename Target, typename... Types>
struct is_type_in;
template <typename Target, typename... Types>
static constexpr bool is_type_in_v = is_type_in<Target, Types...>::value;
} // namespace detail

///
/// \brief Simple sum-type variant/union of Types...
///
template <typename... Types>
	requires(sizeof...(Types) > 0)
class kvariant {
	template <typename V>
	using visit_ret_t = std::invoke_result_t<V, detail::head_of_t<Types...>>;
	template <typename V>
	static constexpr bool visitable_v = (std::is_invocable_v<V, Types> && ...);

  public:
	static constexpr std::size_t size_v = detail::largest_sizeof_v<Types...>;

	// clang-format off
	template <typename T = detail::head_of_t<Types...>>
		requires(detail::is_type_in_v<T, Types...>)
	kvariant(T t = T{}) noexcept(std::is_nothrow_move_constructible_v<T>) { m_storage = std::move(t); }
	// clang-format on

	///
	/// \brief Check which type is currently held
	///
	template <typename T>
		requires(detail::is_type_in_v<T, Types...>)
	bool contains() const noexcept { return m_storage.template contains<T>(); }
	///
	/// \brief Obtain a const pointer to the desired value type if held
	///
	template <typename T>
		requires(detail::is_type_in_v<T, Types...>)
	T const* get_if() const noexcept;
	///
	/// \brief Obtain a const pointer to the desired value type if held
	///
	template <typename T>
		requires(detail::is_type_in_v<T, Types...>)
	T* get_if() noexcept;
	///
	/// \brief Obtain a const lvalue reference to the desired value type (assumed held)
	///
	template <typename T>
		requires(detail::is_type_in_v<T, Types...>)
	T const& get() const& noexcept;
	///
	/// \brief Obtain an lvalue reference to the desired value type (assumed held)
	///
	template <typename T>
		requires(detail::is_type_in_v<T, Types...>)
	T& get() & noexcept;
	///
	/// \brief Obtain an rvalue reference to the desired value type (assumed held)
	///
	template <typename T>
		requires(detail::is_type_in_v<T, Types...>)
	T && get() && noexcept;

	///
	/// \brief Visitor for Types...
	///
	// clang-format off
	template <typename Visitor>
		requires(visitable_v<Visitor>)
	visit_ret_t<Visitor> visit(Visitor&& visitor) const noexcept { return visit<Types...>(m_storage, std::forward<Visitor>(visitor)); }
	// clang-format on

  private:
	template <typename T, typename... Ts, typename Any, typename Visitor>
	static constexpr visit_ret_t<Visitor> visit(Any&& any, Visitor&& visitor) noexcept;

	fixed_any<size_v> m_storage;
};

// impl

namespace detail {
template <typename T>
constexpr T largest(T const a, T const b) noexcept {
	return a < b ? b : a;
}

template <typename T>
struct largest_sizeof<T> {
	static constexpr std::size_t value = sizeof(T);
};

template <typename T, typename... U>
struct largest_sizeof<T, U...> {
	static constexpr std::size_t value = largest(sizeof(T), largest_sizeof<U...>::value);
};

template <typename Target, typename T>
struct is_type_in<Target, T> {
	static constexpr bool value = std::is_same_v<Target, T>;
};

template <typename Target, typename T, typename... Types>
struct is_type_in<Target, T, Types...> {
	static constexpr bool value = is_type_in<Target, T>::value || is_type_in<Target, Types...>::value;
};
} // namespace detail

template <typename... Types>
	requires(sizeof...(Types) > 0)
template <typename T>
	requires(detail::is_type_in_v<T, Types...>)
T const* kvariant<Types...>::get_if() const noexcept {
	if (m_storage.template contains<T>()) { return &m_storage.template get<T>(); }
	return nullptr;
}

template <typename... Types>
	requires(sizeof...(Types) > 0)
template <typename T>
	requires(detail::is_type_in_v<T, Types...>)
T* kvariant<Types...>::get_if() noexcept {
	if (m_storage.template contains<T>()) { return &m_storage.template get<T>(); }
	return nullptr;
}

template <typename... Types>
	requires(sizeof...(Types) > 0)
template <typename T>
	requires(detail::is_type_in_v<T, Types...>)
T const& kvariant<Types...>::get() const& noexcept {
	auto ret = get_if<T>();
	assert(ret);
	return *ret;
}

template <typename... Types>
	requires(sizeof...(Types) > 0)
template <typename T>
	requires(detail::is_type_in_v<T, Types...>)
T& kvariant<Types...>::get() & noexcept {
	auto ret = get_if<T>();
	assert(ret);
	return *ret;
}

template <typename... Types>
	requires(sizeof...(Types) > 0)
template <typename T>
	requires(detail::is_type_in_v<T, Types...>)
T && kvariant<Types...>::get() && noexcept {
	auto ret = get_if<T>();
	assert(ret);
	return std::move(*ret);
}

template <typename... Types>
	requires(sizeof...(Types) > 0)
template <typename T, typename... Ts, typename Any, typename Visitor>
constexpr auto kvariant<Types...>::visit(Any&& any, Visitor&& visitor) noexcept -> visit_ret_t<Visitor> {
	if (any.template contains<T>()) { return visitor(any.template get<T>()); }
	if constexpr (sizeof...(Ts) > 0) { return visit<Ts...>(std::forward<Any>(any), std::forward<Visitor>(visitor)); }
	if constexpr (!std::is_void_v<visit_ret_t<Visitor>>) { return visit_ret_t<Visitor>{}; }
}
} // namespace ktl
