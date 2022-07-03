// KTL header-only library
// Requirements: C++20

#pragma once
#include "unique_val.hpp"
#include <cassert>
#include <functional>
#include <optional>
#include <vector>

namespace ktl {
///
/// \brief Lightweight hash-table with open addressing and reduced iterator stability
///
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class hash_table {
	template <bool Const>
	class iter_t;

  public:
	using key_type = Key;
	using mapped_type = Value;
	using value_type = std::pair<Key, Value>;

	using iterator = iter_t<false>;
	using const_iterator = iter_t<true>;

	static constexpr std::size_t bucket_count_v = 16U;

	hash_table(std::size_t bucket_count = bucket_count_v) { rehash(bucket_count); }
	hash_table(std::initializer_list<value_type> init, std::size_t bucket_count = bucket_count_v);
	template <typename InputIt>
	hash_table(InputIt first, InputIt last, std::size_t bucket_count = bucket_count_v);

	template <typename... Args>
	std::pair<iterator, bool> emplace(Key key, Args&&... args);
	std::pair<iterator, bool> insert_or_assign(Key const& key, Value value);
	std::pair<iterator, bool> insert_or_assign(Key&& key, Value value);
	bool erase(Key const& key);
	iterator erase(iterator it);

	iterator find(Key const& key);
	bool contains(Key const& key) const { return find(key) != end(); }
	const_iterator find(Key const& key) const;
	mapped_type& operator[](Key const& key);

	std::size_t size() const { return m_size; }
	bool empty() const { return size() == 0U; }
	void clear() noexcept;

	iterator begin() noexcept;
	iterator end() { return {&m_table, m_table.size()}; }
	const_iterator begin() const noexcept;
	const_iterator end() const noexcept { return {&m_table, m_table.size()}; }

	void rehash(std::size_t count);
	std::size_t bucket_count() const { return m_table.size(); }
	float load_factor() const;

  private:
	template <typename Table>
	static std::size_t first_bucket_index(Table&& table);
	template <typename K, typename... Args>
	std::pair<iterator, bool> emplace_impl(K&& key, Args&&... args);

	struct node_t;
	using table_t = std::vector<node_t>;

	std::size_t find_node_index(Key const& key, std::size_t& out_visited) const;

	table_t m_table{};
	unique_val<std::size_t> m_size{};
};

// impl

template <typename Key, typename Value, typename Hash>
struct hash_table<Key, Value, Hash>::node_t {
	std::optional<std::pair<Key, Value>> kvp{};
	bool visited{};

	void reset(bool set_visited = {}) {
		kvp.reset();
		visited = set_visited;
	}
};

template <typename Key, typename Value, typename Hash>
template <bool Const>
class hash_table<Key, Value, Hash>::iter_t {
  public:
	using reference = std::pair<Key const&, std::conditional_t<Const, Value const&, Value&>>;
	using difference_type = void;
	using iterator_category = std::forward_iterator_tag;

	struct pointer {
		reference self;
		reference* operator->() { return &self; }
	};

	iter_t() = default;

	reference operator*() const { return {(*m_table)[m_index].kvp->first, (*m_table)[m_index].kvp->second}; }
	pointer operator->() const { return {operator*()}; }

	iter_t& operator++() {
		if (!m_table || m_index >= m_table->size()) { return *this; }
		++m_index;
		while (m_index < m_table->size() && !(*m_table)[m_index].kvp) { ++m_index; }
		return *this;
	}

	iter_t operator++(int) {
		auto ret = *this;
		++(*this);
		return ret;
	}

	operator iter_t<true>() const noexcept { return {m_table, m_index}; }
	bool operator==(iter_t const&) const = default;

  private:
	using table_t = std::conditional_t<Const, typename hash_table::table_t const, typename hash_table::table_t>;

	iter_t(table_t* table, std::size_t index) : m_table(table), m_index(index) {}

	table_t* m_table{};
	std::size_t m_index{};

	friend class hash_table;
	friend class iter_t<false>;
};

template <typename Key, typename Value, typename Hash>
hash_table<Key, Value, Hash>::hash_table(std::initializer_list<value_type> init, std::size_t bucket_count) : hash_table(bucket_count) {
	for (auto const& value : init) { emplace_impl(value.first, value.second); }
}

template <typename Key, typename Value, typename Hash>
template <typename InputIt>
hash_table<Key, Value, Hash>::hash_table(InputIt first, InputIt last, std::size_t bucket_count) : hash_table(bucket_count) {
	for (; first != last; ++first) { emplace_impl(first->first, first->second); }
}

template <typename Key, typename Value, typename Hash>
template <typename... Args>
auto hash_table<Key, Value, Hash>::emplace(Key key, Args&&... args) -> std::pair<iterator, bool> {
	if (auto it = find(key); it != end()) { return {it, false}; }
	return emplace_impl(std::move(key), std::forward<Args>(args)...);
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::insert_or_assign(Key&& key, Value value) -> std::pair<iterator, bool> {
	if (auto it = find(key); it != end()) {
		it->second = std::move(value);
		return {it, false};
	}
	return emplace_impl(std::move(key), std::move(value));
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::insert_or_assign(Key const& key, Value value) -> std::pair<iterator, bool> {
	auto visited = std::size_t{};
	auto index = find_node_index(key, visited);
	if (index == m_table.size()) {
		if (m_table.size() > 5 && visited >= m_table.size() / 5) { rehash(bucket_count()); }
		return emplace_impl(key, std::move(value));
	}
	auto it = iterator{&m_table, index};
	it->second = std::move(value);
	return {it, false};
}

template <typename Key, typename Value, typename Hash>
bool hash_table<Key, Value, Hash>::erase(Key const& key) {
	auto visited = std::size_t{};
	if (auto index = find_node_index(key, visited); index < m_table.size()) {
		m_table[index].reset(true);
		--m_size.value;
		if (m_table.size() > 5 && visited > m_table.size() / 5) { rehash(bucket_count()); }
		return true;
	}
	return false;
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::erase(iterator it) -> iterator {
	if (it == end()) { return it; }
	assert(it.m_table == &m_table && it.m_index < m_table.size() && m_table[it.m_index].kvp);
	m_table[it.m_index].reset(true);
	--m_size.value;
	return ++it;
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::find(Key const& key) -> iterator {
	auto visited = std::size_t{};
	auto index = find_node_index(key, visited);
	if (index == m_table.size()) { return end(); }
	return {&m_table, index};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::find(Key const& key) const -> const_iterator {
	auto visited = std::size_t{};
	auto index = find_node_index(key, visited);
	if (index == m_table.size()) { return end(); }
	return {&m_table, index};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::operator[](Key const& key) -> mapped_type& {
	auto it = find(key);
	if (it == end()) {
		auto [i, _] = emplace_impl(key, Value{});
		it = i;
	}
	return it->second;
}

template <typename Key, typename Value, typename Hash>
void hash_table<Key, Value, Hash>::clear() noexcept {
	for (auto& node : m_table) { node.reset(); }
	m_size = {};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::begin() noexcept -> iterator {
	auto index = first_bucket_index(m_table);
	if (index == bucket_count()) { return end(); }
	return {&m_table, index};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::begin() const noexcept -> const_iterator {
	auto index = first_bucket_index(m_table);
	if (index == bucket_count()) { return end(); }
	return {&m_table, index};
}

template <typename Key, typename Value, typename Hash>
void hash_table<Key, Value, Hash>::rehash(std::size_t count) {
	if (count == 0U) { count = 1U; }

	// make new table
	auto table = table_t(count);
	std::swap(table, m_table);
	m_size = {};

	// move nodes to new table
	for (auto& node : table) {
		if (node.kvp) { emplace_impl(std::move(node.kvp->first), std::move(node.kvp->second)); }
	}
}

template <typename Key, typename Value, typename Hash>
float hash_table<Key, Value, Hash>::load_factor() const {
	auto const buckets = bucket_count();
	if (buckets == 0) { return 1.0f; }
	return static_cast<float>(size()) / static_cast<float>(buckets);
}

template <typename Key, typename Value, typename Hash>
template <typename Table>
std::size_t hash_table<Key, Value, Hash>::first_bucket_index(Table&& table) {
	for (std::size_t i = 0; i < table.size(); ++i) {
		if (table[i].kvp) { return i; }
	}
	return table.size();
}

template <typename Key, typename Value, typename Hash>
template <typename K, typename... Args>
auto hash_table<Key, Value, Hash>::emplace_impl(K&& key, Args&&... args) -> std::pair<iterator, bool> {
	if (load_factor() >= 0.8f) { rehash(bucket_count() * 2U); }
	auto const buckets = bucket_count();
	auto const bucket = Hash{}(key) % buckets;
	auto get_bucket = [&]() {
		auto index = bucket;
		do {
			if (!m_table[index].kvp) { return index; }
			index = (index + 1) % buckets;
		} while (index != bucket);
		assert(false && "invariant violated");
		return index;
	};
	auto const index = get_bucket();
	auto& node = m_table[index];
	node.kvp.emplace(std::pair<Key, Value>(Key{std::forward<K>(key)}, Value{std::forward<Args>(args)...}));
	node.visited = true;
	++m_size.value;
	return {{&m_table, index}, true};
}

template <typename Key, typename Value, typename Hash>
std::size_t hash_table<Key, Value, Hash>::find_node_index(Key const& key, std::size_t& out_visited) const {
	auto const buckets = bucket_count();
	assert(buckets > 0U);
	auto const bucket = Hash{}(key) % buckets;
	auto index = bucket;
	do {
		auto& node = m_table[index];
		if (!node.visited) { return buckets; }
		if (node.kvp && node.kvp->first == key) { return index; }
		++out_visited;
		index = (index + 1) % buckets;
	} while (index != bucket);
	// all nodes marked visited
	return buckets;
}
} // namespace ktl
