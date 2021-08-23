// KTL header-only library
// Requirements: C++17

#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include "kthread.hpp"
#include "move_only_function.hpp"

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
/// \brief Models an async operation via a associated promise, supports .then()
///
template <typename T>
class future;

namespace detail {
template <typename T>
class promise_base_t {
  public:
	promise_base_t() : m_block(std::make_shared<detail::future_block_t<T>>()) {}

	///
	/// \brief Obtain an associated future (multiple instances are supported)
	///
	future<T> get_future() { return future<T>(m_block); }

  protected:
	detail::future_block_ptr<T> m_block;
};
} // namespace detail

///
/// \brief Models an async operation that can deliver the result to a associated future
///
template <typename T>
class promise : public detail::promise_base_t<T> {
  public:
	promise() = default;

	using detail::promise_base_t<T>::get_future;

	///
	/// \brief Set value and signal all associated futures
	///
	template <typename... U, typename = std::enable_if_t<std::is_constructible_v<T, U...>>>
	void set_value(U&&... u);
};

///
/// \brief Models an async operation that can signal the result to a associated future
///
template <>
class promise<void> : public detail::promise_base_t<void> {
  public:
	promise() = default;

	using detail::promise_base_t<void>::get_future;

	///
	/// \brief Signal all associated futures
	///
	void set_value();
};

template <typename T>
class future {
  public:
	future() = default;

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
	future(std::shared_ptr<typename detail::future_block_t<T>> block) : m_block(std::move(block)), m_status(future_status::deferred) {}

	detail::future_block_ptr<T> m_block;
	mutable future_status m_status{};

	friend class detail::promise_base_t<T>;
};

// impl

namespace detail {
template <typename T>
struct future_traits_t {
	using payload_t = std::optional<T>;
	using callback_t = ktl::move_only_function<void(T)>;
};
template <>
struct future_traits_t<void> {
	using payload_t = bool;
	using callback_t = ktl::move_only_function<void()>;
};
template <typename T>
struct future_block_t {
	typename future_traits_t<T>::payload_t payload;
	typename future_traits_t<T>::callback_t then;
	std::mutex mutex;
	std::condition_variable cv;
};
template <typename T>
using future_block_ptr = std::shared_ptr<future_block_t<T>>;
} // namespace detail

template <typename T>
template <typename... U, typename>
void promise<T>::set_value(U&&... u) {
	{
		std::scoped_lock lock(this->m_block->mutex);
		this->m_block->payload.emplace(std::forward<U>(u)...);
		if (this->m_block->then) { this->m_block->then(*this->m_block->payload); }
	}
	this->m_block->cv.notify_all();
}

inline void promise<void>::set_value() {
	{
		std::scoped_lock lock(this->m_block->mutex);
		this->m_block->payload = true;
		if (this->m_block->then) { this->m_block->then(); }
	}
	this->m_block->cv.notify_all();
}

template <typename T>
template <typename Dur>
future_status future<T>::wait_for(Dur duration) const {
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
void future<T>::then(F&& func) {
	assert(m_block);
	std::scoped_lock lock(m_block->mutex);
	m_block->then = std::forward<F>(func);
}

template <typename T>
T future<T>::get() const {
	assert(m_block);
	if (!m_block->payload) {
		std::unique_lock lock(m_block->mutex);
		m_block->cv.wait(lock, [this]() { return m_block->payload; });
	}
	if constexpr (!std::is_void_v<T>) { return *m_block->payload; }
}

template <typename T>
void future<T>::wait() const {
	if (m_block) { get(); }
}
} // namespace ktl
