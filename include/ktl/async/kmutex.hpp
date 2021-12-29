// KTL header-only library
// Requirements: C++17

#pragma once
#include <mutex>
#include <type_traits>

namespace ktl {
///
/// \brief Basic wrapper for a T and its (mutable) mutex
///
template <typename T, typename M = std::mutex>
struct kmutex {
	using type = T;
	using mutex_type = M;

	T t{};
	mutable M mutex;
};
///
/// \brief Strict wrapper for a T and its mutex (T accessible through tlock only)
///
template <typename T, typename M = std::mutex>
class strict_tmutex : kmutex<T, M> {
	template <typename U, template <typename...> typename L, typename V>
	friend class klock;

  public:
	using typename kmutex<T, M>::type;
	using typename kmutex<T, M>::mutex_type;
};
///
/// \brief Wrapper for a scoped mutex lock and the underlying value in a strict_tmutex / tmutex
///
template <typename T, template <typename...> typename L, typename M>
class klock : L<M> {
  public:
	using type = T;
	using lock_type = L<M>;

	template <typename U, typename = std::enable_if_t<std::is_same_v<typename std::decay_t<U>::mutex_type, M>>>
	klock(U&& mutex) : L<M>(mutex.mutex), m_t(std::addressof(mutex.t)) {}

	T& get() const { return *m_t; }
	T& operator*() const { return *m_t; }
	T* operator->() const { return m_t; }

  private:
	T* m_t;
};

///
/// \brief Deduction guide for lvalue argument
///
template <typename T, template <typename...> typename L = std::scoped_lock>
klock(T&) -> klock<typename T::type, L, typename T::mutex_type>;
///
/// \brief Deduction guide for const lvalue argument
///
template <typename T, template <typename...> typename L = std::scoped_lock>
klock(T const&) -> klock<typename T::type const, L, typename T::mutex_type>;
} // namespace ktl
