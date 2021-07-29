// KTL header-only library
// Requirements: C++17

#pragma once
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace ktl {
namespace detail {
template <typename T, typename = T>
struct is_pre_incrementable : std::false_type {};
template <typename T>
struct is_pre_incrementable<T, std::remove_reference_t<decltype(++std::declval<T&>())>> : std::true_type {};
} // namespace detail

///
/// \brief Wrapper over an (un)ordered map
/// Associates each T with a unique RAII handle which can be used to unregister the instance
///
template <typename T, typename Mp = std::unordered_map<std::uint64_t, T>>
class monotonic_map;

template <typename T, typename Mp>
class monotonic_map {
	template <bool IsConst>
	struct iterator_t;

  public:
	using type = T;
	using map_t = Mp;
	using id_t = typename map_t::key_type;
	static_assert(detail::is_pre_incrementable<id_t>::value, "key_type must be pre-incrementable");
	static_assert(std::is_same_v<T, typename map_t::mapped_type>, "T must be mapped_type");

	class handle;
	using iterator = iterator_t<false>;
	using const_iterator = iterator_t<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;

	monotonic_map() = default;
	monotonic_map(monotonic_map&& rhs) noexcept;
	monotonic_map& operator=(monotonic_map&& rhs) noexcept;
	~monotonic_map() noexcept;

	iterator begin() noexcept { return iterator(m_ts.begin()); }
	iterator end() noexcept { return iterator(m_ts.end()); }
	const_iterator begin() const noexcept { return const_iterator(m_ts.begin()); }
	const_iterator end() const noexcept { return const_iterator(m_ts.end()); }
	reverse_iterator rbegin() noexcept { return reverse_iterator(iterator(m_ts.end())); }
	reverse_iterator rend() noexcept { return reverse_iterator(iterator(m_ts.begin())); }

	[[nodiscard]] handle push(T&& t);
	[[nodiscard]] handle push(T const& t);
	T const* find(handle const& h) const noexcept;
	T* find(handle const& h) noexcept;
	std::size_t size() const noexcept { return m_handles.size(); }
	std::size_t clear() noexcept;

  private:
	void destroy() noexcept;
	void move(monotonic_map&& rhs) noexcept;

	std::unordered_set<handle*> m_handles;
	map_t m_ts;
	id_t m_next_id{};

	friend class handle;
};

template <typename T, typename Mp>
class monotonic_map<T, Mp>::handle {
  public:
	constexpr handle() = default;
	handle(handle&& rhs);
	handle& operator=(handle&& rhs);
	~handle() noexcept;

	explicit operator bool() const noexcept { return valid(); }
	bool valid() const noexcept { return m_map && m_id; }
	void reset() noexcept;

  private:
	explicit handle(monotonic_map* map, id_t const* id) : m_map(map), m_id(id) { m_map->m_handles.insert(this); }
	void destroy() noexcept;
	void xfer(handle* rhs) noexcept;

	monotonic_map* m_map{};
	id_t const* m_id{};

	friend class monotonic_map;
};

template <typename T, typename Mp>
template <bool IsConst>
struct monotonic_map<T, Mp>::iterator_t {
	using map_t = typename monotonic_map<T, Mp>::map_t;
	using iter_t = std::conditional_t<IsConst, typename map_t::const_iterator, typename map_t::iterator>;
	using value_type = T;
	using difference_type = typename iter_t::difference_type;
	using pointer = std::conditional_t<IsConst, T const*, T*>;
	using reference = std::conditional_t<IsConst, T const&, T&>;
	using iterator_category = typename iter_t::iterator_category;

	iterator_t() = default;

	bool operator==(iterator_t const& rhs) const noexcept { return iter == rhs.iter; }
	bool operator!=(iterator_t const& rhs) const noexcept { return !(*this == rhs); }

	pointer operator->() { return &iter->second; }
	reference operator*() { return iter->second; }
	iterator_t& operator++() noexcept {
		++iter;
		return *this;
	}
	iterator_t operator++(int) noexcept {
		auto ret = *this;
		++iter;
		return ret;
	}
	iterator_t& operator--() noexcept {
		--iter;
		return *this;
	}
	iterator_t operator--(int) noexcept {
		auto ret = *this;
		--iter;
		return ret;
	}

  private:
	explicit iterator_t(iter_t iter) noexcept : iter(iter) {}

	iter_t iter;

	friend class monotonic_map;
};

// impl

template <typename T, typename Mp>
monotonic_map<T, Mp>::handle::handle(handle&& rhs) : m_map(std::exchange(rhs.m_map, nullptr)), m_id(std::exchange(rhs.m_id, nullptr)) {
	xfer(&rhs);
}

template <typename T, typename Mp>
typename monotonic_map<T, Mp>::handle& monotonic_map<T, Mp>::handle::operator=(handle&& rhs) {
	if (&rhs != this) {
		destroy();
		m_map = std::exchange(rhs.m_map, nullptr);
		m_id = std::exchange(rhs.m_id, nullptr);
		xfer(&rhs);
	}
	return *this;
}

template <typename T, typename Mp>
monotonic_map<T, Mp>::handle::~handle() noexcept {
	destroy();
}

template <typename T, typename Mp>
void monotonic_map<T, Mp>::handle::reset() noexcept {
	destroy();
	m_map = {};
	m_id = {};
}

template <typename T, typename Mp>
void monotonic_map<T, Mp>::handle::destroy() noexcept {
	if (m_map) {
		// TODO: Remove after tests
		assert(m_id && "Invariant violated: map not null but id is null");
		if (m_id) { m_map->m_ts.erase(*m_id); }
		m_map->m_handles.erase(this);
	}
}

template <typename T, typename Mp>
void monotonic_map<T, Mp>::handle::xfer(handle* rhs) noexcept {
	if (m_map) {
		m_map->m_handles.erase(rhs);
		m_map->m_handles.insert(this);
	}
}
template <typename T, typename Mp>
monotonic_map<T, Mp>::monotonic_map(monotonic_map&& rhs) noexcept {
	move(std::move(rhs));
}

template <typename T, typename Mp>
monotonic_map<T, Mp>& monotonic_map<T, Mp>::operator=(monotonic_map&& rhs) noexcept {
	if (&rhs != this) {
		destroy();
		move(std::move(rhs));
	}
	return *this;
}

template <typename T, typename Mp>
monotonic_map<T, Mp>::~monotonic_map() noexcept {
	destroy();
}

template <typename T, typename Mp>
typename monotonic_map<T, Mp>::handle monotonic_map<T, Mp>::push(T&& t) {
	id_t id = ++m_next_id;
	auto [it, _] = m_ts.emplace(std::move(id), std::move(t));
	return handle(this, &it->first);
}

template <typename T, typename Mp>
typename monotonic_map<T, Mp>::handle monotonic_map<T, Mp>::push(T const& t) {
	id_t id = ++m_next_id;
	auto [it, _] = m_ts.emplace(std::move(id), t);
	return handle(this, &it->first);
}

template <typename T, typename Mp>
T const* monotonic_map<T, Mp>::find(handle const& h) const noexcept {
	if (h.m_id) {
		if (auto it = m_ts.find(*h.m_id); it != m_ts.end()) { return &it->second; }
	}
	return nullptr;
}

template <typename T, typename Mp>
T* monotonic_map<T, Mp>::find(handle const& h) noexcept {
	if (h.m_id) {
		if (auto it = m_ts.find(*h.m_id); it != m_ts.end()) { return &it->second; }
	}
	return nullptr;
}

template <typename T, typename Mp>
std::size_t monotonic_map<T, Mp>::clear() noexcept {
	std::size_t const ret = m_handles.size();
	assert(ret == m_ts.size() && "Invariant violated");
	destroy();
	m_handles.clear();
	return ret;
}

template <typename T, typename Mp>
void monotonic_map<T, Mp>::destroy() noexcept {
	m_ts.clear();
	for (auto& h : m_handles) { h->m_map = nullptr; }
}

template <typename T, typename Mp>
void monotonic_map<T, Mp>::move(monotonic_map&& rhs) noexcept {
	m_handles = std::move(rhs.m_handles);
	m_ts = std::move(rhs.m_ts);
	m_next_id = std::exchange(rhs.m_next_id, 0);
	rhs.m_handles.clear();
	rhs.m_ts.clear();
	for (auto& h : m_handles) { h->m_map = this; }
}
} // namespace ktl
