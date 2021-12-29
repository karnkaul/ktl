// KTL single-header library
// Requirements: C++17
//
// Features:
// 	- Multiple queues
// 	- Thread-safe push-and-notify (to any desired queue)
// 	- Thread-safe wait-and-pop (from first of any desired queues)
// 	- Clear all queues and return residue
// 	- Deactivate all queues (as secondary wait condition)
//

#pragma once
#include <cassert>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace ktl {
///
/// \brief Policy customization
///
template <typename M = std::mutex, template <typename> typename Alloc = std::allocator>
struct async_queue_policy {
	template <typename T>
	using queue_t = std::deque<T, Alloc<T>>;
	using mutex_t = M;
};

///
/// \brief FIFO queue with thread safe "sleepy" API
/// \param T value type
/// \param Policy queue policy
///
template <typename T, typename Policy = async_queue_policy<>>
class async_queue {
  public:
	using value_type = T;
	using queue_t = typename Policy::template queue_t<T>;
	using mutex_t = typename Policy::mutex_t;

	///
	/// \brief Queue index (used with multiple queues)
	///
	using queue_id = std::size_t;

	async_queue(std::uint8_t qcount = 1);
	virtual ~async_queue() noexcept { clear(); }

	///
	/// \brief Move a T to the back of desired queue and notify
	///
	void push(T&& t, queue_id qid = 0);
	///
	/// \brief Copy a T to the back of desired queue and notify
	///
	void push(T const& t, queue_id qid = 0);
	///
	/// \brief Emplace a T to the back of desired queue and notify
	///
	template <typename... U>
	void emplace(U&&... u, queue_id qid = 0);
	///
	/// \brief Forward Ts from a container to the back of desired queue and notify
	///
	template <template <typename...> typename Cont, typename... Args>
	void push(Cont<T, Args...>&& ts, queue_id qid = 0);
	///
	/// \brief Pop a T from the front of the first non-empty queue, wait until any populated / not active
	///
	template <template <typename...> typename Cont, typename... Args>
	std::optional<T> pop_any(Cont<queue_id, Args...> qids);
	///
	/// \brief Pop a T from the front of desired queue, wait until populated / not active
	///
	std::optional<T> pop(queue_id qid = 0);
	///
	/// \brief Add a new queue and obtain its qid
	///
	queue_id add_queue();
	///
	/// \brief Flush the queue, notify, and obtain any residual items
	/// \param active Set m_active after moving items
	/// \returns Residual items that were still in queues
	///
	queue_t clear(bool active = false);
	///
	/// \brief Check whether all queues are empty
	///
	bool empty() const;
	///
	/// \brief Check whether instance is active
	///
	bool active() const;
	///
	/// \brief Set active/inactive
	///
	void active(bool value);

  protected:
	// MSVC throws random constexpr failures with C++20 if this is defined out-of-line
	template <template <typename...> typename Cont, typename... Args>
	bool should_wake(Cont<queue_id, Args...> const& qids, queue_t** out) noexcept {
		auto check = [this, out](queue_id qid) {
			queue_t& qu = queue(qid);
			if (!qu.empty()) {
				*out = &qu;
				return true;
			}
			return false;
		};
		if (std::empty(qids)) { return check(0); }
		for (queue_id qid : qids) {
			if (check(qid)) { return true; }
		}
		return false;
	}

	queue_t& queue(queue_id id) noexcept { return m_queues[id]; }
	queue_t const& queue(queue_id id) const noexcept { return m_queues[id]; }

	typename Policy::template queue_t<queue_t> m_queues;
	std::condition_variable m_cv;
	mutable mutex_t m_mutex;
	bool m_active = true;
};

template <typename T, typename Policy>
async_queue<T, Policy>::async_queue(std::uint8_t qcount) {
	if (qcount < 1) { qcount = 1; }
	for (; qcount > 0; --qcount) { add_queue(); }
}

template <typename T, typename Policy>
void async_queue<T, Policy>::push(T&& t, queue_id qid) {
	emplace<T>(std::move(t), qid);
}

template <typename T, typename Policy>
void async_queue<T, Policy>::push(T const& t, queue_id qid) {
	emplace<T>(t, qid);
}

template <typename T, typename Policy>
template <typename... U>
void async_queue<T, Policy>::emplace(U&&... u, queue_id qid) {
	{
		std::scoped_lock lock(m_mutex);
		if (m_active) { queue(qid).emplace_back(std::forward<U>(u)...); }
	}
	m_cv.notify_all();
}

template <typename T, typename Policy>
template <template <typename...> typename C, typename... Args>
void async_queue<T, Policy>::push(C<T, Args...>&& ts, queue_id qid) {
	{
		std::scoped_lock lock(m_mutex);
		if (m_active) { std::move(std::begin(ts), std::end(ts), std::back_inserter(queue(qid))); }
	}
	m_cv.notify_all();
}

template <typename T, typename Policy>
template <template <typename...> typename Cont, typename... Args>
std::optional<T> async_queue<T, Policy>::pop_any(Cont<queue_id, Args...> qids) {
	queue_t* queue{};
	std::unique_lock lock(m_mutex);
	m_cv.wait(lock, [qs = std::move(qids), this, &queue]() -> bool { return !m_active || should_wake(qs, &queue); });
	if (!m_active) { return std::nullopt; }
	assert(queue && !queue->empty());
	auto ret = std::move(queue->front());
	queue->pop_front();
	return ret;
}

template <typename T, typename Policy>
std::optional<T> async_queue<T, Policy>::pop(queue_id qid) {
	std::initializer_list<queue_id> qids = {qid};
	return pop_any(qids);
}

template <typename T, typename Policy>
typename async_queue<T, Policy>::queue_id async_queue<T, Policy>::add_queue() {
	std::scoped_lock lock(m_mutex);
	m_queues.emplace_back();
	return m_queues.size() - 1;
}

template <typename T, typename Policy>
typename async_queue<T, Policy>::queue_t async_queue<T, Policy>::clear(bool active) {
	queue_t ret;
	{
		std::scoped_lock lock(m_mutex);
		m_active = active;
		for (queue_t& queue : m_queues) {
			std::move(std::begin(queue), std::end(queue), std::back_inserter(ret));
			queue.clear();
		}
	}
	m_cv.notify_all();
	return ret;
}

template <typename T, typename Policy>
bool async_queue<T, Policy>::empty() const {
	std::scoped_lock lock(m_mutex);
	for (queue_t const& qu : m_queues) {
		if (!qu.empty()) { return false; }
	}

	return true;
}

template <typename T, typename Policy>
bool async_queue<T, Policy>::active() const {
	std::scoped_lock lock(m_mutex);
	return m_active;
}

template <typename T, typename Policy>
void async_queue<T, Policy>::active(bool set) {
	{
		std::scoped_lock lock(m_mutex);
		m_active = set;
	}
	m_cv.notify_all();
}
} // namespace ktl
