// KTL single-header library
// Requirements: C++20

#pragma once
#include "../kunique_ptr.hpp"

namespace ktl {
///
/// \brief Callable wrapper that cannot be copied, only moved
///
template <typename>
class kfunction;

template <typename R, typename... Args>
class kfunction<R(Args...)> {
	template <typename F>
	static constexpr bool is_mof_v = std::is_same_v<std::remove_cv_t<F>, kfunction<R(Args...)>>;

  public:
	kfunction() = default;
	///
	/// \brief Construct via callable
	///
	template <typename F>
		requires(!is_mof_v<F> && std::is_invocable_r_v<R, F, Args...>)
	kfunction(F f) : m_storage(ktl::make_unique<model_t<F>>(std::move(f))) {}
	///
	/// \brief Assign a callable
	///
	template <typename F>
		requires(!is_mof_v<F> && std::is_invocable_r_v<R, F, Args...>)
	kfunction& operator=(F f) {
		m_storage = ktl::make_unique<model_t<F>>(std::move(f));
		return *this;
	}

	kfunction(kfunction&&) = default;
	kfunction& operator=(kfunction&&) = default;
	kfunction(kfunction const&) = delete;
	kfunction& operator=(kfunction const&) = delete;

	///
	/// \brief Reset assigned callable, if any
	///
	kfunction& operator=(std::nullptr_t) { return (reset(), *this); }
	///
	/// \brief Reset assigned callable, if any
	///
	kfunction& reset() { return (m_storage.reset(), *this); }

	///
	/// \brief Check if a callable has been assigned
	///
	explicit operator bool() const noexcept { return m_storage != nullptr; }
	///
	/// \brief Check if a callable has been assigned
	///
	bool has_value() const noexcept { return m_storage != nullptr; }
	///
	/// \brief Invoke assigned callable (assumed present)
	///
	R operator()(Args... args) const { return m_storage->call(args...); }

  private:
	struct base_t {
		virtual ~base_t() = default;
		virtual R call(Args... args) = 0;
	};
	template <typename F>
	struct model_t : base_t {
		F func;
		template <typename... T>
		model_t(T&&... t) : func(std::forward<T>(t)...) {}
		R call(Args... args) override { return func(args...); }
	};
	ktl::kunique_ptr<base_t> m_storage;
};
} // namespace ktl
