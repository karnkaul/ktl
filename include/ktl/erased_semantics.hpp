// KTL header-only library
// Requirements: C++17

#pragma once
#include <new>
#include <type_traits>
#include <utility>

namespace ktl {
///
/// \brief Basic type eraser that supports move/copy construct/assign and destroy semantics
///
class erased_semantics {
	template <typename T>
	static constexpr bool can_copy_v = std::is_copy_constructible_v<T>&& std::is_copy_assignable_v<T>;

  public:
	template <typename T>
	struct tag_t {};

	template <typename T>
	constexpr erased_semantics(tag_t<T>) noexcept;

	void move_construct(void* src, void* dst) const noexcept { m_fop(oper::move_c, src, dst); }
	void move_assign(void* src, void* dst) const noexcept { m_fop(oper::move_a, src, dst); }
	void copy_construct(void const* src, void* dst) const { m_fop(oper::copy_c, const_cast<void*>(src), dst); }
	void copy_assign(void const* src, void* dst) const { m_fop(oper::copy_a, const_cast<void*>(src), dst); }
	void destroy(void const* src) const noexcept { m_fop(oper::destruct, const_cast<void*>(src), nullptr); }

  private:
	enum class oper { move_c, move_a, copy_c, copy_a, destruct };
	using fop = void (*)(oper op, void* src, void* dst);

	template <typename T>
	static void operate(oper op, void* src, void* dst = {});

	fop const m_fop;
};

// impl

template <typename T>
constexpr erased_semantics::erased_semantics(tag_t<T>) noexcept : m_fop(&operate<T>) {
	static_assert(can_copy_v<T>, "T must be copiable");
}

template <typename T>
void erased_semantics::operate(oper op, void* src, void* dst) {
	switch (op) {
	case oper::move_c: new (dst) T(std::move(*std::launder(reinterpret_cast<T*>(src)))); break;
	case oper::move_a: *std::launder(reinterpret_cast<T*>(dst)) = std::move(*std::launder(reinterpret_cast<T*>(src))); break;
	case oper::copy_c: new (dst) T(*std::launder(reinterpret_cast<T const*>(src))); break;
	case oper::copy_a: *std::launder(reinterpret_cast<T*>(dst)) = *std::launder(reinterpret_cast<T const*>(src)); break;
	case oper::destruct: std::launder(reinterpret_cast<T const*>(src))->~T(); break;
	}
}
} // namespace ktl
