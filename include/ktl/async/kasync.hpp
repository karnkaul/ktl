// KTL single-header library
// Requirements: C++20

#pragma once
#include <algorithm>
#include <vector>
#include "kfuture.hpp"
#include "kmutex.hpp"

namespace ktl {
///
/// \brief RAII wrapper for asynchronous invocation (blocks until drained on destruction)
///
class kasync {
  public:
	template <typename F, typename... Args>
	using Ret = std::invoke_result_t<F, Args...>;

	kasync() = default;
	~kasync() { ktl::klock(m_threads)->clear(); }

	///
	/// \brief Enqueue callable in thread pool and obtain future
	///
	template <typename F, typename... Args>
	ktl::kfuture<Ret<F, Args...>> operator()(F&& f, Args... args) {
		auto task = ktl::kpackaged_task<Ret<F, Args...>(Args...)>(std::forward<F>(f));
		auto ret = task.get_future();
		auto lock = ktl::klock(m_threads);
		std::erase_if(*lock, [](ktl::kthread const& thread) { return !thread.active(); });
		lock->push_back(ktl::kthread(std::move(task), std::move(args)...));
		return ret;
	}

  private:
	ktl::strict_tmutex<std::vector<ktl::kthread>> m_threads;
};
} // namespace ktl
