// KTL single-header library
// Requirements: C++20

#pragma once
#include "koverloaded.hpp"
#include <cassert>
#include <new>
#include <type_traits>
#include <utility>

namespace ktl {
///
/// \brief RAII union of two types
///
template <typename T, typename U>
class either {
	template <typename V>
	static constexpr bool visitable_v = std::is_invocable_v<V, T>&& std::is_invocable_v<V, U>;
	template <typename V>
	using visit_ret_t = std::invoke_result_t<V, T>;
	static constexpr bool noexcept_movable_v = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<U>;
	static constexpr bool noexcept_copiable_v = std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_constructible_v<U>;

  public:
	template <typename Ty>
	using resolve_t = std::conditional_t<std::is_same_v<std::decay_t<Ty>, U>, U, std::conditional_t<std::is_same_v<std::decay_t<Ty>, T>, T, void>>;
	template <typename Ty>
	static constexpr bool valid_v = !std::is_void_v<resolve_t<Ty>>;

	///
	/// \brief (Implicitly) construct via T or U
	///
	template <typename Ty = T, typename = std::enable_if_t<valid_v<Ty>>>
	constexpr either(Ty&& t = T{}) noexcept(std::is_nothrow_constructible_v<resolve_t<Ty>, Ty>);

	constexpr either(either&& rhs) noexcept(noexcept_movable_v) : either() { exchg(*this, rhs); }
	constexpr either(either const& rhs) noexcept(noexcept_copiable_v);
	constexpr either& operator=(either rhs) noexcept(noexcept_movable_v) { return (exchg(*this, rhs), *this); }
	constexpr ~either() noexcept { destroy(); }

	///
	/// \brief Check which type is currently held
	///
	template <typename Ty>
	constexpr bool contains() const noexcept;
	///
	/// \brief Obtain a const lvalue reference to the desired value type (assumed held)
	///
	template <typename Ty>
	constexpr Ty const& get() const& noexcept;
	///
	/// \brief Obtain an lvalue reference to the desired value type (assumed held)
	///
	template <typename Ty>
	constexpr Ty& get() & noexcept;
	///
	/// \brief Obtain an rvalue reference to the desired value type (assumed held)
	///
	template <typename Ty>
	constexpr Ty&& get() && noexcept;
	///
	/// \brief Obtain a const pointer to the desired value type if held
	///
	template <typename Ty>
	constexpr Ty const* get_if() const noexcept;
	///
	/// \brief Obtain a pointer to the desired value type if held
	///
	template <typename Ty>
	constexpr Ty* get_if() noexcept;
	///
	/// \brief Set either out_t or out_u based on held value
	///
	constexpr void set(T& out_t, U& out_u) const noexcept;
	///
	/// \brief Visitor for T and U
	///
	template <typename Visitor>
		requires(visitable_v<Visitor>)
	constexpr visit_ret_t<Visitor> visit(Visitor&& func) const noexcept { return visit(*this, std::forward<Visitor>(func)); }

  private:
	static constexpr void exchg(either& lhs, either& rhs) noexcept(noexcept_movable_v);
	static constexpr void asymm_exchg(either& tsrc, either& usrc) noexcept(noexcept_movable_v);

	template <typename E, typename V>
	static constexpr visit_ret_t<V> visit(E&& either, V&& visitor) noexcept;
	template <typename Ty, typename... Args>
	static constexpr void construct(Ty* ptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty, Args...>) {
		new (ptr) Ty(std::forward<Args>(args)...);
	}
	template <typename Ty>
	static constexpr void destruct(Ty const* ptr) noexcept {
		ptr->~Ty();
	}
	constexpr void destroy() noexcept;

	union {
		T t_;
		U u_;
	};
	bool m_u = false;
};

// impl

template <typename T, typename U>
template <typename Ty, typename>
constexpr either<T, U>::either(Ty&& t) noexcept(std::is_nothrow_constructible_v<resolve_t<Ty>, Ty>) {
	if constexpr (std::is_same_v<std::decay_t<Ty>, T>) {
		construct(&t_, std::forward<Ty>(t));
	} else {
		construct(&u_, std::forward<Ty>(t));
		m_u = true;
	}
}

template <typename T, typename U>
constexpr either<T, U>::either(either const& rhs) noexcept(noexcept_copiable_v) {
	if (rhs.m_u) {
		construct(&u_, rhs.u_);
	} else {
		construct(&t_, rhs.t_);
	}
	m_u = rhs.m_u;
}

template <typename T, typename U>
template <typename Ty>
constexpr bool either<T, U>::contains() const noexcept {
	static_assert(valid_v<Ty>);
	if constexpr (std::is_same_v<Ty, T>) {
		return !m_u;
	} else {
		return m_u;
	}
}

template <typename T, typename U>
template <typename Ty>
constexpr Ty const& either<T, U>::get() const& noexcept {
	static_assert(valid_v<Ty>);
	assert(contains<Ty>());
	if constexpr (std::is_same_v<Ty, T>) {
		return t_;
	} else {
		return u_;
	}
}

template <typename T, typename U>
template <typename Ty>
constexpr Ty& either<T, U>::get() & noexcept {
	static_assert(valid_v<Ty>);
	assert(contains<Ty>());
	if constexpr (std::is_same_v<Ty, T>) {
		return t_;
	} else {
		return u_;
	}
}

template <typename T, typename U>
template <typename Ty>
constexpr Ty&& either<T, U>::get() && noexcept {
	static_assert(valid_v<Ty>);
	assert(contains<Ty>());
	if constexpr (std::is_same_v<Ty, T>) {
		return std::move(t_);
	} else {
		return std::move(u_);
	}
}

template <typename T, typename U>
template <typename Ty>
constexpr Ty const* either<T, U>::get_if() const noexcept {
	static_assert(valid_v<Ty>);
	if constexpr (std::is_same_v<Ty, T>) {
		return contains<T>() ? &t_ : nullptr;
	} else {
		return contains<U>() ? &u_ : nullptr;
	}
}

template <typename T, typename U>
template <typename Ty>
constexpr Ty* either<T, U>::get_if() noexcept {
	static_assert(valid_v<Ty>);
	if constexpr (std::is_same_v<Ty, T>) {
		return contains<T>() ? &t_ : nullptr;
	} else {
		return contains<U>() ? &u_ : nullptr;
	}
}

template <typename T, typename U>
constexpr void either<T, U>::set(T& out_t, U& out_u) const noexcept {
	if (contains<T>()) {
		out_t = get<T>();
	} else {
		out_u = get<U>();
	}
}

template <typename T, typename U>
template <typename E, typename V>
constexpr auto either<T, U>::visit(E&& either, V&& visitor) noexcept -> visit_ret_t<V> {
	if (either.template contains<T>()) {
		return visitor(either.template get<T>());
	} else {
		return visitor(either.template get<U>());
	}
}

template <typename T, typename U>
constexpr void either<T, U>::exchg(either& lhs, either& rhs) noexcept(noexcept_movable_v) {
	if (lhs.m_u && rhs.m_u) {
		std::swap(lhs.u_, rhs.u_);
	} else if (!lhs.m_u && !rhs.m_u) {
		std::swap(lhs.t_, rhs.t_);
	} else if (rhs.m_u) {
		asymm_exchg(lhs, rhs);
	} else {
		asymm_exchg(rhs, lhs);
	}
	std::swap(lhs.m_u, rhs.m_u);
}

template <typename T, typename U>
constexpr void either<T, U>::asymm_exchg(either& tsrc, either& usrc) noexcept(noexcept_movable_v) {
	T t = std::move(tsrc.t_);
	U u = std::move(usrc.u_);
	destruct(&tsrc.t_);
	destruct(&usrc.u_);
	construct(&usrc.t_, std::move(t));
	construct(&tsrc.u_, std::move(u));
}

template <typename T, typename U>
constexpr void either<T, U>::destroy() noexcept {
	if (m_u) {
		destruct(&u_);
	} else {
		destruct(&t_);
	}
}
} // namespace ktl
