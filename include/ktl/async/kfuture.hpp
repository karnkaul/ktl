// KTL header-only library
// Requirements: C++20

#pragma once
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>
#include "kfunction.hpp"
#include "kthread.hpp"

namespace ktl {
namespace detail {
template <typename T>
struct future_block_t;
template <typename T>
using future_block_ptr = std::shared_ptr<future_block_t<T>>;
} // namespace detail

///
/// \brief enumeration of future statuses
///
enum class future_status { idle, deferred, ready };

///
/// \brief Models an async operation via a associated promise, supports .then(); T must be copiable
///
template <typename T>
class kfuture;

namespace detail {
template <typename T>
class promise_base_t {
  public:
	promise_base_t() : m_block(std::make_shared<detail::future_block_t<T>>()) {}

	///
	/// \brief Obtain an associated future (multiple instances are supported)
	///
	kfuture<T> get_future() { return kfuture<T>(m_block); }

  protected:
	detail::future_block_ptr<T> m_block;
};
} // namespace detail

///
/// \brief Models an async operation that can deliver the result to a associated future
///
template <typename T>
class kpromise : public detail::promise_base_t<T> {
  public:
	kpromise() = default;

	using detail::promise_base_t<T>::get_future;

	///
	/// \brief Set value and signal all associated futures
	///
	template <typename... U>
		requires(std::is_constructible_v<T, U...>)
	void set_value(U&&... u);
};

///
/// \brief Models an async operation that can signal the result to a associated future
///
template <>
class kpromise<void> : public detail::promise_base_t<void> {
  public:
	kpromise() = default;

	using detail::promise_base_t<void>::get_future;

	///
	/// \brief Signal all associated futures
	///
	void set_value();
};

template <typename T>
class kfuture {
  public:
	kfuture() = default;

	///
	/// \brief Obtain future status after waiting for max duration
	///
	template <typename Dur>
	future_status wait_for(Dur duration) const;
	///
	/// \brief Enqueue a callback for when future is ready
	///
	template <typename F>
	void then(F&& func);
	///
	/// \brief Block this thread until future is signalled
	///
	T get() const;
	///
	/// \brief Block until ready
	///
	void wait() const;
	///
	/// \brief Check whether this instance points to some shared state
	///
	bool valid() const noexcept { return m_block != nullptr; }
	///
	/// \brief Check whether shared state, if any, is ready
	///
	bool ready() const { return wait_for(std::chrono::milliseconds()) == future_status::ready; }
	///
	/// \brief Check whether shared state, if any, is busy
	///
	bool busy() const { return wait_for(std::chrono::milliseconds()) == future_status::deferred; }

  private:
	kfuture(std::shared_ptr<typename detail::future_block_t<T>> block) : m_block(std::move(block)), m_status(future_status::deferred) {}

	detail::future_block_ptr<T> m_block;
	mutable future_status m_status{};

	friend class detail::promise_base_t<T>;
};

///
/// \brief Wrapper for invocable and promise
///
template <typename F, typename... Args>
class kpackaged_task;

///
/// \brief Wrapper for invocable and promise
///
template <typename R, typename... Args>
class kpackaged_task<R(Args...)> {
  public:
	kpackaged_task() = default;
	///
	/// \brief Construct via invocable
	///
	template <typename F>
	kpackaged_task(F f);

	///
	/// \brief Check if invocation is pending
	///
	bool valid() const noexcept { return m_func; }
	explicit operator bool() const noexcept { return valid(); }

	kfuture<R> get_future() { return m_promise.get_future(); }
	///
	/// \brief Discard and reset shared and invocation state
	///
	void reset();

	///
	/// \brief Invoke stored callable (assumed valid) and signal associated future(s)
	///
	void operator()(Args... args);

  private:
	kfunction<R(Args...)> m_func;
	kpromise<R> m_promise;
};

// impl

namespace detail {
template <typename T>
struct future_traits_t {
	using payload_t = std::optional<T>;
	using callback_t = kfunction<void(T)>;
};
template <>
struct future_traits_t<void> {
	using payload_t = bool;
	using callback_t = kfunction<void()>;
};
template <typename T>
struct future_block_t {
	typename future_traits_t<T>::payload_t payload;
	std::vector<typename future_traits_t<T>::callback_t> thens;
	std::mutex mutex;
	std::condition_variable cv;
};
template <typename T>
using future_block_ptr = std::shared_ptr<future_block_t<T>>;
} // namespace detail

template <typename T>
template <typename... U>
	requires(std::is_constructible_v<T, U...>)
void kpromise<T>::set_value(U&&... u) {
	{
		std::scoped_lock lock(this->m_block->mutex);
		this->m_block->payload.emplace(std::forward<U>(u)...);
		for (auto const& then : this->m_block->thens) { then(*this->m_block->payload); }
	}
	this->m_block->cv.notify_all();
}

inline void kpromise<void>::set_value() {
	{
		std::scoped_lock lock(this->m_block->mutex);
		this->m_block->payload = true;
		for (auto const& then : this->m_block->thens) { then(); }
	}
	this->m_block->cv.notify_all();
}

template <typename T>
template <typename Dur>
future_status kfuture<T>::wait_for(Dur duration) const {
	if (m_status == future_status::deferred) {
		auto const begin = std::chrono::steady_clock::now();
		do {
			std::scoped_lock lock(m_block->mutex);
			if (m_block->payload) {
				m_status = future_status::ready;
				break;
			}
		} while (std::chrono::steady_clock::now() - begin < duration);
	}
	return m_status;
}

template <typename T>
template <typename F>
void kfuture<T>::then(F&& func) {
	assert(m_block);
	std::scoped_lock lock(m_block->mutex);
	m_block->thens.push_back(std::forward<F>(func));
}

template <typename T>
T kfuture<T>::get() const {
	assert(m_block);
	if (!m_block->payload) {
		std::unique_lock lock(m_block->mutex);
		m_block->cv.wait(lock, [this]() { return m_block->payload; });
	}
	if constexpr (!std::is_void_v<T>) { return *m_block->payload; }
}

template <typename T>
void kfuture<T>::wait() const {
	if (m_block) { get(); }
}

template <typename R, typename... Args>
template <typename F>
kpackaged_task<R(Args...)>::kpackaged_task(F f)
	: m_func([f = std::move(f)](Args... args) {
		  if constexpr (std::is_void_v<R>) {
			  f(std::move(args)...);
		  } else {
			  return f(std::move(args)...);
		  }
	  }) {}

template <typename R, typename... Args>
void kpackaged_task<R(Args...)>::reset() {
	m_func = {};
	m_promise = {};
}

template <typename R, typename... Args>
void kpackaged_task<R(Args...)>::operator()(Args... args) {
	assert(m_func);
	if constexpr (std::is_void_v<R>) {
		m_func(std::move(args)...);
		m_promise.set_value();
	} else {
		m_promise.set_value(m_func(std::move(args)...));
	}
	reset();
}
} // namespace ktl
