#pragma once
#include <algorithm>
#include "future.hpp"
#include "tmutex.hpp"

namespace ktl {
///
/// \brief RAII wrapper for asynchronous invocation (blocks until drained on destruction)
///
class async {
  public:
	template <typename F, typename... Args>
	using Ret = std::invoke_result_t<F, Args...>;

	async() = default;
	~async() { ktl::tlock(m_threads)->clear(); }

	///
	/// \brief Enqueue callable in thread pool and obtain future
	///
	template <typename F, typename... Args>
	ktl::future<Ret<F, Args...>> operator()(F&& f, Args... args) {
		auto task = ktl::packaged_task<Ret<F, Args...>(Args...)>(std::forward<F>(f));
		auto ret = task.get_future();
		auto lock = ktl::tlock(m_threads);
		lock->erase(std::remove_if(lock->begin(), lock->end(), [](ktl::kthread const& thread) { return !thread.active(); }), lock->end());
		lock->push_back(ktl::kthread(std::move(task), std::move(args)...));
		return ret;
	}

  private:
	ktl::strict_tmutex<std::vector<ktl::kthread>> m_threads;
};
} // namespace ktl
