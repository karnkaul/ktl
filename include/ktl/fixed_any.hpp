// KTL header-only library
// Requirements: C++20

#pragma once
#include <cassert>
#include <concepts>
#include <cstddef>
#include <stdexcept>

namespace ktl {
namespace detail {
struct fixed_any_vtable {
	void (*construct)(void* ptr);
	void (*move)(void* src, void* dst);
	void (*copy)(void const* src, void* dst);
	void (*destroy)(void const* ptr);
};

template <typename T>
constexpr fixed_any_vtable fixed_any_vtable_v = {
	[](void* ptr) { new (ptr) T{}; },
	[](void* src, void* dst) { *static_cast<T*>(dst) = std::move(*static_cast<T*>(src)); },
	[](void const* src, void* dst) { *static_cast<T*>(dst) = *static_cast<T const*>(src); },
	[](void const* ptr) { static_cast<T const*>(ptr)->~T(); },
};
} // namespace detail

///
/// \brief Fixed-size type erased storage
///
template <std::size_t Capacity = sizeof(void*)>
class fixed_any final {
	template <typename T>
	static constexpr bool is_different_v = !std::is_same_v<T, fixed_any<Capacity>>;
	template <typename T>
	static constexpr bool is_copiable_v = std::is_copy_constructible_v<std::decay_t<T>>;

  public:
	constexpr fixed_any() noexcept = default;

	constexpr fixed_any(fixed_any&& rhs) noexcept;
	constexpr fixed_any(fixed_any const& rhs);
	constexpr fixed_any& operator=(fixed_any&& rhs) noexcept;
	constexpr fixed_any& operator=(fixed_any const& rhs);
	constexpr ~fixed_any() noexcept;

	///
	/// \brief Construct with object of type T
	///
	template <typename T>
		requires(is_different_v<T>&& is_copiable_v<T>)
	constexpr fixed_any(T t) noexcept(std::is_nothrow_move_constructible_v<T>) { construct(std::move(t)); }
	///
	/// \brief Assign to object of type T
	///
	template <typename T>
		requires(is_different_v<T>&& is_copiable_v<T>)
	constexpr fixed_any& operator=(T t) { return (construct(std::move(t)), *this); }
	///
	/// \brief Check if held type (if any) matches T
	///
	template <typename T>
	constexpr bool contains() const noexcept;
	///
	/// \brief Check if no type is held
	///
	constexpr bool empty() const noexcept;
	///
	/// \brief Obtain reference to T
	/// Throws / returns static reference on type mismatch
	///
	template <typename T>
	constexpr T const& get() const;
	///
	/// \brief Obtain reference to T
	/// Throws / returns static reference on type mismatch
	///
	template <typename T>
	constexpr T& get();
	///
	/// \brief Obtain a copy of T if contained, else fallback
	/// Throws / returns static reference on type mismatch
	///
	template <typename T>
	constexpr T value_or(T const& fallback) const;

	///
	/// \brief Destroy held type (if any)
	///
	constexpr bool clear() noexcept;

  private:
	template <typename T>
	constexpr void construct(T t);

	constexpr void assign(detail::fixed_any_vtable const* vtable) {
		if (m_vtable != vtable) {
			clear();
			m_vtable = vtable;
			if (m_vtable) { m_vtable->construct(&m_bytes); }
		}
	}

	std::aligned_storage_t<Capacity, alignof(std::max_align_t)> m_bytes;
	detail::fixed_any_vtable const* m_vtable{};
};

template <std::size_t Capacity>
constexpr fixed_any<Capacity>::fixed_any(fixed_any&& rhs) noexcept : m_vtable(rhs.m_vtable) {
	if (m_vtable) {
		m_vtable->construct(&m_bytes);
		m_vtable->move(&rhs.m_bytes, &m_bytes);
	}
}

template <std::size_t Capacity>
constexpr fixed_any<Capacity>::fixed_any(fixed_any const& rhs) : m_vtable(rhs.m_vtable) {
	if (m_vtable) {
		m_vtable->construct(&m_bytes);
		m_vtable->copy(&rhs.m_bytes, &m_bytes);
	}
}

template <std::size_t Capacity>
constexpr fixed_any<Capacity>& fixed_any<Capacity>::operator=(fixed_any&& rhs) noexcept {
	if (&rhs != this) {
		assign(rhs.m_vtable);
		if (m_vtable) { m_vtable->move(&rhs.m_bytes, &m_bytes); }
	}
	return *this;
}

template <std::size_t Capacity>
constexpr fixed_any<Capacity>& fixed_any<Capacity>::operator=(fixed_any const& rhs) {
	if (&rhs != this) {
		assign(rhs.m_vtable);
		if (m_vtable) { m_vtable->copy(&rhs.m_bytes, &m_bytes); }
	}
	return *this;
}

template <std::size_t Capacity>
constexpr fixed_any<Capacity>::~fixed_any() noexcept {
	clear();
}

template <std::size_t Capacity>
template <typename T>
constexpr bool fixed_any<Capacity>::contains() const noexcept {
	return &detail::fixed_any_vtable_v<T> == m_vtable;
}

template <std::size_t Capacity>
constexpr bool fixed_any<Capacity>::empty() const noexcept {
	return m_vtable == nullptr;
}

template <std::size_t Capacity>
template <typename T>
constexpr T const& fixed_any<Capacity>::get() const {
	if (contains<T>()) { return *std::launder(reinterpret_cast<T const*>(&m_bytes)); }
	throw std::runtime_error("fixed_any_t: Type mismatch!");
}

template <std::size_t Capacity>
template <typename T>
constexpr T& fixed_any<Capacity>::get() {
	if (contains<T>()) { return *std::launder(reinterpret_cast<T*>(&m_bytes)); }
	throw std::runtime_error("fixed_any_t: Type mismatch!");
}

template <std::size_t Capacity>
template <typename T>
constexpr T fixed_any<Capacity>::value_or(T const& fallback) const {
	if (contains<T>()) { return *std::launder(reinterpret_cast<T const*>(&m_bytes)); }
	return fallback;
}

template <std::size_t Capacity>
constexpr bool fixed_any<Capacity>::clear() noexcept {
	if (m_vtable) {
		m_vtable->destroy(&m_bytes);
		m_vtable = {};
		return true;
	}
	return false;
}

template <std::size_t Capacity>
template <typename T>
constexpr void fixed_any<Capacity>::construct(T t) {
	if constexpr (std::is_same_v<T, std::nullptr_t>) {
		clear();
	} else {
		static_assert(is_different_v<T>, "fixed_any_t: Recursive storage is forbidden");
		static_assert(sizeof(std::decay_t<T>) <= Capacity, "fixed_any_t: T is too large (compared to N)");
		static_assert(alignof(std::decay_t<T>) <= alignof(std::max_align_t), "fixed_any_t: alignof(T) is too large");
		assign(&detail::fixed_any_vtable_v<T>);
		m_vtable->move(&t, &m_bytes);
	}
}
} // namespace ktl
