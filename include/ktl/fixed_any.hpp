// KTL header-only library
// Requirements: C++20

#pragma once
#include <concepts>
#include <cstddef>
#include <stdexcept>

namespace ktl {
struct fixed_any_vtable {
	void (*const move)(void* src, void* dst);
	void (*const copy)(void const* src, void* dst);
	void (*const destroy)(void const* ptr);
};

template <typename T>
fixed_any_vtable const& get_fixed_any_vtable() {
	static auto const ret = fixed_any_vtable{
		[](void* src, void* dst) { new (dst) T(std::move(*static_cast<T*>(src))); },
		[](void const* src, void* dst) { new (dst) T(*static_cast<T const*>(src)); },
		[](void const* ptr) { static_cast<T const*>(ptr)->~T(); },
	};
	return ret;
}

///
/// \brief Fixed-size type erased storage
///
template <std::size_t Capacity = sizeof(void*), std::size_t Align = alignof(std::max_align_t)>
class fixed_any final {
	template <typename T>
	static constexpr bool is_different_v = !std::is_same_v<T, fixed_any<Capacity>>;

  public:
	fixed_any() = default;

	///
	/// \brief Move construct T
	///
	template <typename T>
		requires(is_different_v<T>)
	fixed_any(T t) { emplace<T>(std::move(t)); }

	fixed_any(fixed_any&& rhs) noexcept { move(std::move(rhs)); }
	fixed_any(fixed_any const& rhs) { copy(rhs); }
	fixed_any& operator=(fixed_any&& rhs) noexcept { return (move(std::move(rhs)), *this); }
	fixed_any& operator=(fixed_any const& rhs) { return (copy(rhs), *this); }
	~fixed_any() noexcept { clear(); }

	///
	/// \brief Construct T via Args...
	///
	template <typename T, typename... Args>
		requires(std::is_copy_constructible_v<T> && sizeof(T) <= Capacity && alignof(T) <= Align)
	T& emplace(Args&&... args);

	///
	/// \brief Check if held type (if any) matches T
	///
	template <typename T>
	bool contains() const {
		return m_vtable == &get_fixed_any_vtable<T>();
	}
	///
	/// \brief Obtain reference to T
	/// Throws on type mismatch
	///
	template <typename T>
	T const& get() const;
	///
	/// \brief Obtain reference to T
	/// Throws on type mismatch
	///
	template <typename T>
	T& get();
	///
	/// \brief Obtain reference to T if contained, else fallback
	///
	template <typename T>
	T const& value_or(T const& fallback) const;
	///
	/// \brief Check if no type is held
	///
	bool empty() const { return m_vtable == nullptr; }
	///
	/// \brief Destroy held type (if any)
	///
	bool clear();

	fixed_any_vtable const* vtable() const { return m_vtable; }
	std::byte const* data() const { return m_data; }

  private:
	void move(fixed_any&& rhs) {
		clear();
		m_vtable = rhs.m_vtable;
		if (m_vtable) { m_vtable->move(rhs.m_data, m_data); }
	}

	void copy(fixed_any const& rhs) {
		clear();
		m_vtable = rhs.m_vtable;
		if (m_vtable) { m_vtable->copy(rhs.m_data, m_data); }
	}

	alignas(Align) std::byte m_data[Capacity]{};
	fixed_any_vtable const* m_vtable{};
};

template <std::size_t Capacity, std::size_t Align>
template <typename T, typename... Args>
	requires(std::is_copy_constructible_v<T> && sizeof(T) <= Capacity && alignof(T) <= Align)
T& fixed_any<Capacity, Align>::emplace(Args&&... args) {
	clear();
	auto ret = new (m_data) T(std::forward<Args>(args)...);
	m_vtable = &get_fixed_any_vtable<T>();
	return *ret;
}

template <std::size_t Capacity, std::size_t Align>
template <typename T>
T const& fixed_any<Capacity, Align>::get() const {
	if (contains<T>()) { return *std::launder(reinterpret_cast<T const*>(m_data)); }
	throw std::runtime_error("fixed_any_t: Type mismatch!");
}

template <std::size_t Capacity, std::size_t Align>
template <typename T>
T& fixed_any<Capacity, Align>::get() {
	return const_cast<T&>(static_cast<fixed_any const&>(*this).get<T>());
}

template <std::size_t Capacity, std::size_t Align>
template <typename T>
T const& fixed_any<Capacity, Align>::value_or(T const& fallback) const {
	if (contains<T>()) { return *std::launder(reinterpret_cast<T const*>(m_data)); }
	return fallback;
}

template <std::size_t Capacity, std::size_t Align>
bool fixed_any<Capacity, Align>::clear() {
	if (m_vtable) {
		m_vtable->destroy(m_data);
		m_vtable = {};
		return true;
	}
	return false;
}
} // namespace ktl
