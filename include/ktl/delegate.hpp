// KTL header-only library
// Requirements: C++17

#pragma once
#include "move_only_function.hpp"
#include "tagged_store.hpp"

namespace ktl {
namespace detail {
template <typename... Args>
using delegate_callback = move_only_function<void(Args const&...)>;
} // namespace detail

///
/// \brief Store for observers and RAII handles
///
template <typename T, typename StorePolicy = tagged_store_policy>
class observer_store;

///
/// \brief Delegate with customizable store policy
///
template <typename StorePolicy, typename... Args>
class policy_delegate;

///
/// \brief Delegate: store for callbacks; provides RAII signal instances
///
template <typename... Args>
using delegate = policy_delegate<tagged_store_policy, Args...>;

template <typename T, typename StorePolicy>
class observer_store : public tagged_store<T, StorePolicy> {
  public:
	using store_t = tagged_store<T, StorePolicy>;
	using tag_t = typename store_t::tag_t;

	///
	/// \brief RAII handle for attaching / detaching Ts
	///
	class handle;

	constexpr observer_store() = default;
	constexpr observer_store(observer_store&& rhs) noexcept : observer_store() { exchg(*this, rhs); }
	constexpr observer_store& operator=(observer_store&& rhs) noexcept { return (exchg(*this, rhs), *this); }
	constexpr ~observer_store() noexcept { clear(); }

	[[nodiscard]] constexpr tag_t attach(T t) { return this->push(std::move(t)); }
	constexpr bool detach(tag_t tag) { return this->pop(tag); }
	[[nodiscard]] constexpr handle make_handle() noexcept { return this; }

	constexpr void clear() noexcept;

  private:
	static constexpr void exchg(observer_store& lhs, observer_store& rhs) noexcept;

	constexpr void exchg(handle const* existing, handle* replace) noexcept;
	constexpr void track(handle* signal) { m_handles.push_back(signal); }
	constexpr void untrack(handle const* signal) { m_handles.erase(std::remove(m_handles.begin(), m_handles.end(), signal)); }

	typename StorePolicy::template store_t<handle*> m_handles;

	friend class handle;
};

template <typename T, typename StorePolicy>
class observer_store<T, StorePolicy>::handle {
  public:
	constexpr handle() = default;
	constexpr handle(handle&& rhs) noexcept : handle() { exchg(*this, rhs); }
	constexpr handle& operator=(handle&& rhs) noexcept { return (exchg(*this, rhs), *this); }
	constexpr ~handle();

	constexpr bool active() const noexcept { return m_delegate != nullptr; }
	explicit constexpr operator bool() const noexcept { return active(); }

	constexpr tag_t attach(T t);
	constexpr bool detach(tag_t tag);
	constexpr bool replace(tag_t tag, T t) noexcept(std::is_nothrow_move_assignable_v<T>);
	constexpr void clear() noexcept;
	constexpr bool operator+=(T t) { return attach(std::move(t)); }

	constexpr tag_t tag(std::size_t index = 0) const noexcept { return index < m_tags.size() ? m_tags[index] : tag_t{}; }

  private:
	static constexpr void exchg(handle& lhs, handle& rhs) noexcept;

	constexpr handle(observer_store* del) : m_delegate(del) { m_delegate->track(this); }

	typename StorePolicy::template store_t<tag_t> m_tags;
	observer_store* m_delegate{};

	friend class observer_store;
};

template <typename StorePolicy, typename... Args>
class policy_delegate : public observer_store<detail::delegate_callback<Args...>> {
  public:
	using policy_t = StorePolicy;
	using callback = move_only_function<void(Args const&...)>;
	using signal = typename observer_store<callback>::handle;

	[[nodiscard]] signal make_signal() { return this->make_handle(); }

	void operator()(Args const&... t) const { dispatch(t...); }
	void dispatch(Args const&... t) const {
		for (callback const& cb : *this) { cb(t...); }
	}
};

// impl

template <typename T, typename StorePolicy>
constexpr void observer_store<T, StorePolicy>::clear() noexcept {
	for (auto& h : m_handles) { h->m_delegate = {}; }
	m_handles.clear();
	store_t::clear();
}

template <typename T, typename StorePolicy>
constexpr void observer_store<T, StorePolicy>::exchg(observer_store& lhs, observer_store& rhs) noexcept {
	for (auto& h : lhs.m_handles) { h->m_delegate = &rhs; }
	for (auto& h : rhs.m_handles) { h->m_delegate = &lhs; }
	std::swap(lhs.m_handles, rhs.m_handles);
	std::swap(static_cast<store_t&>(lhs), static_cast<store_t&>(rhs));
}

template <typename T, typename StorePolicy>
constexpr void observer_store<T, StorePolicy>::exchg(handle const* existing, handle* replace) noexcept {
	for (auto& s : m_handles) {
		if (s == existing) {
			s = replace;
			break;
		}
	}
}

template <typename T, typename StorePolicy>
constexpr observer_store<T, StorePolicy>::handle::~handle() {
	clear();
	if (m_delegate) { m_delegate->untrack(this); }
}

template <typename T, typename StorePolicy>
constexpr auto observer_store<T, StorePolicy>::handle::attach(T t) -> tag_t {
	if (active()) {
		auto const ret = m_delegate->attach(std::move(t));
		m_tags.push_back(ret);
		return ret;
	}
	return StorePolicy::null_tag_v;
}

template <typename T, typename StorePolicy>
constexpr bool observer_store<T, StorePolicy>::handle::detach(tag_t tag) {
	if (active() && m_delegate->detach(tag)) {
		m_tags.erase(std::remove(m_tags.begin(), m_tags.end(), tag), m_tags.end());
		return true;
	}
	return false;
}

template <typename T, typename StorePolicy>
constexpr bool observer_store<T, StorePolicy>::handle::replace(tag_t tag, T t) noexcept(std::is_nothrow_move_assignable_v<T>) {
	auto const it = std::find(m_tags.begin(), m_tags.end(), tag);
	if (active() && it != m_tags.end()) {
		if (auto p = m_delegate->find(tag)) {
			*p = std::move(t);
			return true;
		}
	}
	return false;
}

template <typename T, typename StorePolicy>
constexpr void observer_store<T, StorePolicy>::handle::clear() noexcept {
	if (active() && !m_tags.empty()) {
		for (auto const tag : m_tags) { m_delegate->detach(tag); }
	}
}

template <typename T, typename StorePolicy>
constexpr void observer_store<T, StorePolicy>::handle::exchg(handle& lhs, handle& rhs) noexcept {
	if (lhs.m_delegate) { lhs.m_delegate->exchg(&lhs, &rhs); }
	if (rhs.m_delegate) { rhs.m_delegate->exchg(&rhs, &lhs); }
	std::swap(lhs.m_delegate, rhs.m_delegate);
	std::swap(lhs.m_tags, rhs.m_tags);
}
} // namespace ktl
