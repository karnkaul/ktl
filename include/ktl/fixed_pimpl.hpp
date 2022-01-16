// KTL header-only library
// Requirements: C++20

#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace ktl {
template <typename T, std::size_t Size, std::size_t Align = alignof(std::max_align_t)>
class fixed_pimpl {
  public:
	using type = T;
	using storage_t = std::aligned_storage_t<Size, Align>;

	static constexpr std::size_t size_v = Size;
	static constexpr std::size_t align_v = Align;

	fixed_pimpl() noexcept(std::is_nothrow_move_constructible_v<T>) { construct(T{}); }
	fixed_pimpl(T&& t) noexcept(std::is_nothrow_move_constructible_v<T>) { construct(std::move(t)); }
	fixed_pimpl(T const& t) noexcept(std::is_nothrow_copy_constructible_v<T>) { construct(t); }
	fixed_pimpl(fixed_pimpl&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>) : fixed_pimpl(T{}) { get() = std::move(rhs.get()); }
	fixed_pimpl& operator=(fixed_pimpl&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>) { return (get() = std::move(rhs.get()), *this); }
	fixed_pimpl(fixed_pimpl const& rhs) = delete;
	fixed_pimpl& operator=(fixed_pimpl const& rhs) = delete;
	~fixed_pimpl() noexcept { reinterpret_cast<T*>(&m_storage)->~T(); }

	T const& get() const noexcept { return *reinterpret_cast<T const*>(&m_storage); }
	T& get() noexcept { return *reinterpret_cast<T*>(&m_storage); }
	T const& operator*() const noexcept { return get(); }
	T& operator*() noexcept { return get(); }
	T const* operator->() const noexcept { return &get(); }
	T* operator->() noexcept { return &get(); }

  private:
	template <typename U>
	void construct(U&& u) noexcept(std::is_nothrow_constructible_v<T, U>) {
		static_assert(sizeof(T) <= Size && alignof(T) <= Align);
		new (&m_storage) T(std::forward<U>(u));
	}

	storage_t m_storage;
};
} // namespace ktl
