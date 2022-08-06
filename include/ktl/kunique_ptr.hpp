// KTL header-only library
// Requirements: C++20

#pragma once
#include <cassert>
#include <concepts>
#include <utility>

namespace ktl {
///
/// \brief Lightweight unique pointer to heap-allocated Type*
///
template <typename Type>
class kunique_ptr {
	static_assert(!std::is_array_v<Type> && !std::is_reference_v<Type>);
	Type* m_ptr{};

  public:
	using element_type = Type;
	using pointer = Type*;

	kunique_ptr() = default;
	explicit kunique_ptr(Type* ptr) : m_ptr(ptr) {}

	kunique_ptr(kunique_ptr&& rhs) noexcept : kunique_ptr() { swap(rhs); }
	kunique_ptr& operator=(kunique_ptr&& rhs) noexcept { return (swap(rhs), *this); }

	template <std::derived_from<Type> T>
	kunique_ptr(kunique_ptr<T>&& rhs) noexcept : kunique_ptr(rhs.release()) {}
	template <std::derived_from<Type> T>
	kunique_ptr& operator=(kunique_ptr<T>&& rhs) noexcept;

	~kunique_ptr() noexcept { reset(); }

	Type* release() noexcept { return std::exchange(m_ptr, {}); }
	void reset(Type* ptr = nullptr) noexcept;
	void swap(kunique_ptr& rhs) noexcept;

	explicit operator bool() const noexcept { return m_ptr; }
	Type* get() const noexcept { return m_ptr; }

	Type& operator*() const { return (assert(m_ptr), *m_ptr); }
	Type* operator->() const { return (assert(m_ptr), m_ptr); }

	bool operator==(kunique_ptr const& rhs) const noexcept;
	bool operator==(std::nullptr_t) const noexcept { return !m_ptr; }
};

template <typename T, typename... Args>
kunique_ptr<T> make_unique(Args&&... args) {
	return kunique_ptr<T>(new T(std::forward<Args>(args)...));
}

// impl

template <typename Type>
void kunique_ptr<Type>::reset(Type* ptr) noexcept {
	auto old = std::exchange(m_ptr, ptr);
	if (old) { delete old; }
}

template <typename Type>
void kunique_ptr<Type>::swap(kunique_ptr& rhs) noexcept {
	using std::swap;
	swap(m_ptr, rhs.m_ptr);
}

template <typename Type>
template <std::derived_from<Type> T>
kunique_ptr<Type>& kunique_ptr<Type>::operator=(kunique_ptr<T>&& rhs) noexcept {
	m_ptr = rhs.release();
	return *this;
}

template <typename Type>
bool kunique_ptr<Type>::operator==(kunique_ptr const& rhs) const noexcept {
	return (!m_ptr && !rhs.m_ptr) || (*m_ptr == *rhs.m_ptr);
}
} // namespace ktl
