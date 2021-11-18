// KTL single-header library
// Requirements: C++17

#pragma once
#include <memory>
#include <type_traits>

namespace ktl {
///
/// \brief Callable wrapper that cannot be copied, only moved
///
template <typename>
class move_only_function;

template <typename R, typename... Args>
class move_only_function<R(Args...)> {
	template <typename F>
	using enable_if_not_mof_t = std::enable_if_t<!std::is_same_v<std::decay_t<F>, move_only_function<R(Args...)>>>;

  public:
	move_only_function() = default;
	///
	/// \brief Construct via callable
	///
	template <typename F, typename = enable_if_not_mof_t<F>>
	move_only_function(F f) : m_storage(std::make_unique<model_t<F>>(std::move(f))) {}
	///
	/// \brief Assign a callable
	///
	template <typename F, typename = enable_if_not_mof_t<F>>
	move_only_function& operator=(F&& f) {
		m_storage = std::make_unique<model_t<F>>(std::forward<F>(f));
		return *this;
	}

	///
	/// \brief Reset assigned callable, if any
	///
	move_only_function& operator=(std::nullptr_t) { return (reset(), *this); }
	///
	/// \brief Reset assigned callable, if any
	///
	move_only_function& reset() { return (m_storage.reset(), *this); }

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
	R operator()(Args... args) const { return m_storage->call(std::move(args)...); }

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
		R call(Args... args) override {
			if constexpr (std::is_void_v<R>) {
				func(std::move(args)...);
			} else {
				return func(std::move(args)...);
			}
		}
	};
	std::unique_ptr<base_t> m_storage;
};

} // namespace ktl
