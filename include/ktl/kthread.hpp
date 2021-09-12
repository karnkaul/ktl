// KTL single-header library
// Requirements: C++17

#pragma once
#include <atomic>
#include <memory>
#include <thread>
#include <type_traits>

namespace ktl {
///
/// \brief std::thread wrapper that joins on destruction / move, and supports stop_t tokens
///
class kthread {
  public:
	enum class policy { wait, stop };

	class stop_t;

	template <typename F, typename... Args>
	using invocable_t = std::enable_if_t<std::is_invocable_v<F, Args...> || std::is_invocable_v<F, stop_t, Args...>>;

	///
	/// \brief Yield execution of the calling thread
	///
	static void yield() { std::this_thread::yield(); }
	///
	/// \brief Sleep calling thread for a specific duration (approximate)
	///
	template <typename Dur>
	static void sleep_for(Dur&& duration);

	kthread() = default;
	///
	/// \brief Invoke F(Args...) or F(stop_t, Args...) on a new thread
	///
	template <typename F, typename... Args, typename = invocable_t<F, Args...>>
	explicit kthread(F&& func, Args&&... args);
	kthread(kthread&& rhs) noexcept : kthread() { exchg(*this, rhs); }
	kthread& operator=(kthread rhs) noexcept { return (exchg(*this, rhs), *this); }
	virtual ~kthread() { join(); }

	///
	/// \brief Join the thread wrapped in this instance, blocking the calling thread
	///
	bool join();
	///
	/// \brief Swap this instance with rhs
	///
	void swap(kthread& rhs) noexcept;
	///
	/// \brief Signal stop token (if existent)
	///
	bool request_stop() noexcept;
	///
	/// \brief Check if an execution context is running
	///
	bool active() const noexcept { return m_thread.joinable(); }
	///
	/// \brief Whether to send stop signal before joining
	///
	policy m_join = policy::wait;

  protected:
	void exchg(kthread& lhs, kthread& rhs) noexcept;

  private:
	std::thread m_thread;
	std::unique_ptr<std::atomic_bool> m_stop;
};

///
/// \brief Stop Token
///
class kthread::stop_t {
  public:
	bool stop_requested() const noexcept { return m_stop && m_stop->load(); }

  private:
	explicit stop_t(std::atomic_bool* stop) noexcept : m_stop(stop) {}
	std::atomic_bool* m_stop;
	friend class kthread;
};

// impl

template <typename Dur>
void kthread::sleep_for(Dur&& duration) {
	std::this_thread::sleep_for(duration);
}
template <typename F, typename... Args, typename>
inline kthread::kthread(F&& func, Args&&... args) {
	if constexpr (std::is_invocable_v<F, stop_t, Args...>) {
		m_stop = std::make_unique<std::atomic_bool>(false);
		m_thread = std::thread(std::forward<F>(func), stop_t(m_stop.get()), std::forward<Args>(args)...);
	} else {
		m_thread = std::thread(std::forward<F>(func), std::forward<Args>(args)...);
	}
}
inline void kthread::swap(kthread& rhs) noexcept {
	std::swap(m_thread, rhs.m_thread);
	std::swap(m_stop, rhs.m_stop);
}
inline bool kthread::request_stop() noexcept {
	if (m_stop) {
		bool b = false;
		return m_stop->compare_exchange_strong(b, true);
	}
	return false;
}
inline bool kthread::join() {
	if (m_thread.joinable()) {
		if (m_join == policy::stop) { request_stop(); }
		m_thread.join();
		m_stop.reset();
		return true;
	}
	return false;
}
inline void kthread::exchg(kthread& lhs, kthread& rhs) noexcept {
	std::swap(lhs.m_thread, rhs.m_thread);
	std::swap(lhs.m_stop, rhs.m_stop);
	std::swap(lhs.m_join, rhs.m_join);
}
} // namespace ktl
