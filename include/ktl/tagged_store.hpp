// KTL header-only library
// Requirements: C++17

#pragma once
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

namespace ktl {
struct tagged_store_policy {
	template <typename T>
	using store_t = std::vector<T>;
	using tag_t = std::uint64_t;
	static constexpr tag_t null_tag_v = tag_t{};
};

///
/// \brief Storage for individually tagged Ts; supports popping T via associated tag and bidirectional iteration
///
template <typename T, typename Policy = tagged_store_policy>
class tagged_store {
	template <bool Const>
	class iter_t;

  public:
	using policy_t = Policy;
	using tag_t = typename Policy::tag_t;
	using iterator = iter_t<false>;
	using const_iterator = iter_t<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	static constexpr tag_t null_tag_v = Policy::null_tag_v;

	[[nodiscard]] constexpr tag_t push(T t);
	constexpr bool pop(tag_t tag);
	constexpr T* find(tag_t tag) noexcept;
	constexpr T const* find(tag_t tag) const noexcept;

	constexpr std::size_t size() const noexcept { return m_store.size(); }
	constexpr void clear() noexcept { m_store.clear(); }
	constexpr bool empty() const noexcept { return m_store.empty(); }

	constexpr iterator begin() noexcept { return m_store.begin(); }
	constexpr iterator end() noexcept { return m_store.end(); }
	constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(iterator(m_store.end())); }
	constexpr reverse_iterator rend() noexcept { return reverse_iterator(iterator(m_store.begin())); }
	constexpr const_iterator cbegin() noexcept { return m_store.cbegin(); }
	constexpr const_iterator cend() noexcept { return m_store.cend(); }
	constexpr const_iterator begin() const noexcept { return m_store.begin(); }
	constexpr const_iterator end() const noexcept { return m_store.end(); }
	constexpr const_reverse_iterator crbegin() noexcept { return const_reverse_iterator(const_iterator(m_store.end())); }
	constexpr const_reverse_iterator crend() noexcept { return const_reverse_iterator(const_iterator(m_store.begin())); }
	constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(const_iterator(m_store.end())); }
	constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(const_iterator(m_store.begin())); }

  private:
	struct entry_t {
		T t;
		tag_t tag = null_tag_v;
	};
	using store_t = typename Policy::template store_t<entry_t>;

	store_t m_store;
	tag_t m_next = null_tag_v;
};

template <typename T, typename Policy>
template <bool Const>
class tagged_store<T, Policy>::iter_t {
	using iterator_t = std::conditional_t<Const, typename store_t::const_iterator, typename store_t::iterator>;

  public:
	using iterator_category = std::bidirectional_iterator_tag;
	using type = T;
	using value_type = T;
	using difference_type = typename iterator_t::difference_type;
	using pointer = std::conditional_t<Const, T const*, T*>;
	using reference = std::conditional_t<Const, T const&, T&>;

	constexpr iter_t() = default;

	constexpr reference operator*() const noexcept { return m_it->t; }
	constexpr pointer operator->() const noexcept { return &m_it->t; }

	constexpr iter_t& operator++() noexcept { return (++m_it, *this); }
	constexpr iter_t& operator--() noexcept { return (--m_it, *this); }
	constexpr iter_t operator++(int) noexcept {
		auto ret = *this;
		++m_it;
		return ret;
	}
	constexpr iter_t operator--(int) noexcept {
		auto ret = *this;
		--m_it;
		return ret;
	}

	friend constexpr bool operator==(iter_t lhs, iter_t rhs) noexcept { return lhs.m_it == rhs.m_it; }
	friend constexpr bool operator!=(iter_t lhs, iter_t rhs) noexcept { return !(lhs == rhs); }

  private:
	constexpr iter_t(iterator_t it) noexcept : m_it(it) {}

	iterator_t m_it;

	friend class tagged_store;
};

// impl

template <typename T, typename Policy>
constexpr auto tagged_store<T, Policy>::push(T t) -> tag_t {
	tag_t ret = ++m_next;
	m_store.push_back({std::move(t), ret});
	return ret;
}

template <typename T, typename Policy>
constexpr bool tagged_store<T, Policy>::pop(tag_t tag) {
	auto it = std::remove_if(m_store.begin(), m_store.end(), [tag](entry_t const& e) { return e.tag == tag; });
	if (it != m_store.end()) {
		m_store.erase(it, m_store.end());
		return true;
	}
	return false;
}

template <typename T, typename Policy>
constexpr T* tagged_store<T, Policy>::find(tag_t tag) noexcept {
	auto it = std::find_if(m_store.begin(), m_store.end(), [tag](entry_t const& e) { return e.tag == tag; });
	return it != m_store.end() ? &it->t : nullptr;
}

template <typename T, typename Policy>
constexpr T const* tagged_store<T, Policy>::find(tag_t tag) const noexcept {
	auto it = std::find_if(m_store.begin(), m_store.end(), [tag](entry_t const& e) { return e.tag == tag; });
	return it != m_store.end() ? &it->t : nullptr;
}
} // namespace ktl
