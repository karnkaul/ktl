// KTL header-only library
// Requirements: C++17

#pragma once
#include <condition_variable>
#include <functional>
#include <memory>
#include <optional>
#include <ktl/kthread.hpp>

namespace ktl {
namespace detail {
template <typename T>
struct future_traits_t {
	using payload_t = std::optional<T>;
	using callback_t = std::function<void(T)>;
};
template <>
struct future_traits_t<void> {
	using payload_t = bool;
	using callback_t = std::function<void()>;
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
class future_t;

template <typename T>
class promise_base_t {
  public:
	promise_base_t() : m_block(std::make_shared<detail::future_block_t<T>>()) {}

	future_t<T> get_future() { return future_t<T>(m_block); }

  protected:
	std::shared_ptr<detail::future_block_t<T>> m_block;
};

template <typename T>
class promise_t : public promise_base_t<T> {
  public:
	promise_t() = default;

	template <typename... U, typename = std::enable_if_t<std::is_constructible_v<T, U...>>>
	void set_value(U&&... u) {
		{
			std::unique_lock lock(this->m_block->mutex);
			this->m_block->payload.emplace(std::forward<U>(u)...);
			if (this->m_block->then) { this->m_block->then(*this->m_block->payload); }
		}
		this->m_block->cv.notify_one();
	}
};

template <>
class promise_t<void> : public promise_base_t<void> {
  public:
	promise_t() = default;

	void set_value() {
		{
			std::unique_lock lock(this->m_block->mutex);
			this->m_block->payload = true;
			if (this->m_block->then) { this->m_block->then(); }
		}
		this->m_block->cv.notify_one();
	}
};

enum class future_status { idle, deferred, ready };

template <typename T>
class future_t {
  public:
	future_t() = default;

	template <typename Dur>
	future_status wait_for(Dur duration) {
		if (m_status == future_status::deferred) {
			auto const begin = std::chrono::steady_clock::now();
			do {
				std::unique_lock lock(m_block->mutex);
				if (m_block->payload) {
					m_status = future_status::ready;
					break;
				}
			} while (std::chrono::steady_clock::now() - begin < duration);
		}
		return m_status;
	}

	T get() {
		assert(m_block);
		if (!m_block->payload) {
			std::unique_lock lock(m_block->mutex);
			m_block->cv.wait(lock, [this]() { return m_block->payload; });
		}
		if constexpr (!std::is_void_v<T>) { return *m_block->payload; }
	}

	template <typename F>
	void then(F&& func) {
		assert(m_block);
		std::unique_lock lock(m_block->mutex);
		m_block->then = std::forward<F>(func);
	}

	bool valid() const noexcept { return m_block != nullptr; }
	bool ready() const noexcept { return m_status == future_status::ready; }

  private:
	future_t(std::shared_ptr<typename detail::future_block_t<T>> block) : m_block(std::move(block)), m_status(future_status::deferred) {}

	std::shared_ptr<typename detail::future_block_t<T>> m_block;
	future_status m_status{};

	friend class promise_base_t<T>;
};
} // namespace ktl
