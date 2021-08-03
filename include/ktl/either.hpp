// KTL single-header library
// Requirements: C++17

#pragma once
#include <cassert>
#include <type_traits>
#include <utility>

namespace ktl {
///
/// \brief Wrapper for visitor
///
template <typename... T>
struct overloaded : T... {
	using T::operator()...;
};
template <typename... T>
overloaded(T...) -> overloaded<T...>;

///
/// \brief RAII union of two types
///
template <typename T, typename U>
class either {
	template <typename Ty>
	static constexpr bool visitable_v = std::is_invocable_v<Ty, T const&>&& std::is_invocable_v<Ty, U const&>;

  public:
	template <typename Ty>
	using resolve_t = std::conditional_t<std::is_same_v<std::decay_t<Ty>, U>, U, std::conditional_t<std::is_same_v<std::decay_t<Ty>, T>, T, void>>;
	template <typename Ty>
	static constexpr bool valid_v = !std::is_void_v<resolve_t<Ty>>;

	template <typename Ty = T, typename = std::enable_if_t<valid_v<Ty>>>
	constexpr either(Ty&& t = T{}) noexcept(std::is_nothrow_constructible_v<resolve_t<Ty>, Ty>);
	constexpr either(either<T, U>&& rhs) noexcept : m_u(rhs.m_u) { grab<true>(std::move(rhs)); }
	constexpr either(either<T, U> const& rhs) : m_u(rhs.m_u) { grab<false>(rhs); }
	constexpr either& operator=(either<T, U>&& rhs) noexcept;
	constexpr either& operator=(either<T, U> const& rhs);
	~either() noexcept { destroy(); }

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
	template <typename F, typename = std::enable_if_t<visitable_v<F>>>
	constexpr void visit(F&& func) const noexcept;

  private:
	template <typename Ty, typename... Args>
	constexpr void construct(Ty* ptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<Ty, Args...>) {
		new (ptr) Ty(std::forward<Args>(args)...);
	}
	template <typename Ty>
	constexpr void destruct(Ty const* ptr) noexcept {
		ptr->~Ty();
	}
	void destroy();
	template <bool NE, typename Ty>
	constexpr void grab(Ty&& rhs) noexcept(NE);

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
	if constexpr (std::is_same_v<Ty, T>) {
		construct(&t_, std::forward<Ty>(t));
	} else {
		construct(&u_, std::forward<Ty>(t));
		m_u = true;
	}
}

template <typename T, typename U>
constexpr either<T, U>& either<T, U>::operator=(either<T, U>&& rhs) noexcept {
	if (&rhs != this) {
		destroy();
		m_u = rhs.m_u;
		grab<true>(std::move(rhs));
	}
	return *this;
}

template <typename T, typename U>
constexpr either<T, U>& either<T, U>::operator=(either<T, U> const& rhs) {
	if (&rhs != this) {
		destroy();
		m_u = rhs.m_u;
		grab<false>(rhs);
	}
	return *this;
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
template <typename F, typename>
constexpr void either<T, U>::visit(F&& func) const noexcept {
	if (contains<T>()) {
		func(get<T>());
	} else {
		func(get<U>());
	}
}

template <typename T, typename U>
void either<T, U>::destroy() {
	if (m_u) {
		destruct(&u_);
	} else {
		destruct(&t_);
	}
}

template <typename T, typename U>
template <bool NE, typename Ty>
constexpr void either<T, U>::grab(Ty&& rhs) noexcept(NE) {
	if (rhs.m_u) {
		construct(&u_, std::move(rhs.u_));
	} else {
		construct(&t_, std::move(rhs.t_));
	}
}
} // namespace ktl
