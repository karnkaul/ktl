// KTL header-only library
// Requirements: C++20

#pragma once
#include "kunique_ptr.hpp"
#include "unique_val.hpp"
#include <cassert>
#include <functional>
#include <optional>
#include <vector>

namespace ktl {
///
/// \brief Lightweight hash-table with chaining and reduced iterator stability
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

	hash_table(std::size_t count = 16) { rehash(count); }

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
	void clear() noexcept;

	iterator begin() noexcept;
	iterator end() { return {m_table, m_table.size(), {}}; }
	const_iterator begin() const noexcept;
	const_iterator end() const noexcept { return {m_table, m_table.size(), {}}; }

	void rehash(std::size_t count);
	std::size_t bucket_count() const { return m_table.size(); }
	std::size_t bucket(Key const& key) const { return Hash{}(key) % m_table.size(); }
	float load_factor() const { return static_cast<float>(size()) / static_cast<float>(bucket_count()); }

  private:
	template <typename Table>
	static std::size_t first_bucket_index(Table&& table);
	template <typename NodePtr>
	static std::pair<NodePtr, NodePtr> find_node_and_parent(Key const& key, NodePtr root, NodePtr parent = {});
	template <typename K, typename... Args>
	std::pair<iterator, bool> emplace_impl(K&& key, Args&&... args);

	struct node_t;
	using table_t = std::vector<node_t>;

	void insert_node(node_t* node);

	table_t m_table{};
	unique_val<std::size_t> m_size{};
};

// impl

template <typename Key, typename Value, typename Hash>
struct hash_table<Key, Value, Hash>::node_t {
	std::optional<std::pair<Key, Value>> kvp{};
	ktl::kunique_ptr<node_t> next{};

	node_t() = default;
	node_t(std::pair<Key, Value> kvp) noexcept : kvp(std::move(kvp)) {}

	node_t(node_t&&) = default;
	node_t& operator=(node_t&&) = default;

	node_t(node_t const& rhs) { copy_from(rhs); }
	node_t& operator=(node_t const& rhs) {
		if (&rhs != this) {
			next.reset();
			copy_from(rhs);
		}
		return *this;
	}

	void copy_from(node_t const& rhs) {
		kvp = rhs.kvp;
		auto src = rhs.next.get();
		auto* dst = &next;
		while (src) {
			assert(src->kvp);
			*dst = ktl::make_unique<node_t>(*src->kvp);
			dst = &(*dst)->next;
			src = src->next.get();
		}
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

	reference operator*() const { return {m_node->kvp->first, m_node->kvp->second}; }
	pointer operator->() const { return {operator*()}; }

	iter_t& operator++() {
		if (m_root == m_bend) { return *this; }

		// go to next node
		assert(m_node);
		if (m_node->next) {
			m_node = m_node->next.get();
			return *this;
		}

		// go to next bucket or end
		m_node = {};
		do { ++m_root; } while (m_root != m_bend && !m_root->kvp);
		if (m_root != m_bend) { m_node = &*m_root; }
		return *this;
	}

	iter_t operator++(int) {
		auto ret = *this;
		++(*this);
		return ret;
	}

	operator iter_t<true>() const noexcept { return {m_root, m_bend, m_node}; }
	bool operator==(iter_t const&) const = default;

  private:
	using table_ref_t = std::conditional_t<Const, typename hash_table::table_t const&, typename hash_table::table_t&>;
	using table_it_t = std::conditional_t<Const, typename hash_table::table_t::const_iterator, typename hash_table::table_t::iterator>;
	using node_ptr_t = std::conditional_t<Const, node_t const*, node_t*>;

	iter_t(table_ref_t table, std::size_t bidx, node_ptr_t node) noexcept : m_root(table.begin() + bidx), m_bend(table.end()), m_node(node) {}
	iter_t(table_it_t table, table_it_t tend, node_ptr_t node) : m_root(table), m_bend(tend), m_node(node) {}

	table_it_t m_root{};
	table_it_t m_bend{};
	node_ptr_t m_node{};

	friend class hash_table;
	friend class iter_t<false>;
};

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
	if (auto it = find(key); it != end()) {
		it->second = std::move(value);
		return {it, false};
	}
	return emplace_impl(key, std::move(value));
}

template <typename Key, typename Value, typename Hash>
bool hash_table<Key, Value, Hash>::erase(Key const& key) {
	node_t& root = m_table[bucket(key)];
	auto const [node, parent] = find_node_and_parent(key, &root);
	if (!node) { return false; }

	if (!parent) {
		// target is root node
		assert(node == &root);
		if (root.next) {
			root = std::move(*root.next);
		} else {
			root.kvp.reset();
		}
	} else {
		// target has parent node
		parent->next = std::move(node->next);
	}

	--m_size.value;
	return true;
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::erase(iterator it) -> iterator {
	if (it == end()) { return it; }

	// obtain bucket index
	assert(it.m_node && it.m_node->kvp);
	auto bidx = static_cast<std::size_t>(it.m_root - m_table.begin());
	assert(bidx < m_table.size());
	node_t* node = it.m_node;

	--m_size.value;
	if (node->next) {
		// replace node with child
		*node = std::move(*node->next);
		return {m_table, bidx, node};
	}
	// reset kvp, find next node to return
	node->kvp.reset();
	for (; bidx < m_table.size(); ++bidx) {
		node = &m_table[bidx];
		if (node->kvp) { break; }
	}

	if (bidx == m_table.size()) { return end(); }
	assert(node->kvp);
	return {m_table, bidx, node};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::find(Key const& key) -> iterator {
	auto const bidx = bucket(key);
	auto const [node, _] = find_node_and_parent(key, &m_table[bidx]);
	if (node) { return {m_table, bidx, node}; }
	return end();
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::find(Key const& key) const -> const_iterator {
	auto const bidx = bucket(key);
	auto const [node, _] = find_node_and_parent(key, &m_table[bidx]);
	if (node) { return {m_table, bidx, node}; }
	return end();
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::operator[](Key const& key) -> mapped_type& {
	auto it = find(key);
	assert(it != end());
	return it->second;
}

template <typename Key, typename Value, typename Hash>
void hash_table<Key, Value, Hash>::clear() noexcept {
	m_table.clear();
	m_size = {};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::begin() noexcept -> iterator {
	auto bidx = first_bucket_index(m_table);
	if (bidx == m_table.size()) { return end(); }
	return {m_table, bidx, &m_table[bidx]};
}

template <typename Key, typename Value, typename Hash>
auto hash_table<Key, Value, Hash>::begin() const noexcept -> const_iterator {
	auto bidx = first_bucket_index(m_table);
	if (bidx == m_table.size()) { return end(); }
	return {m_table, bidx, &m_table[bidx]};
}

template <typename Key, typename Value, typename Hash>
void hash_table<Key, Value, Hash>::rehash(std::size_t count) {
	if (count == 0U) { count = 1U; }
	if (m_table.size() >= count) { return; }

	// make new table
	auto table = table_t(count);
	std::swap(table, m_table);
	m_size = {};

	// move nodes to new table
	for (auto& root : table) {
		if (root.kvp) { insert_node(&root); }
	}
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
template <typename NodePtr>
std::pair<NodePtr, NodePtr> hash_table<Key, Value, Hash>::find_node_and_parent(Key const& key, NodePtr root, NodePtr parent) {
	if (root) {
		if (root->kvp && root->kvp->first == key) { return {root, parent}; }
		return find_node_and_parent<NodePtr>(key, root->next.get(), root);
	}
	return {};
}

template <typename Key, typename Value, typename Hash>
void hash_table<Key, Value, Hash>::insert_node(node_t* node) {
	assert(node->kvp);
	emplace_impl(std::move(node->kvp->first), std::move(node->kvp->second));
	if (auto next = node->next.get()) { insert_node(next); }
}

template <typename Key, typename Value, typename Hash>
template <typename K, typename... Args>
auto hash_table<Key, Value, Hash>::emplace_impl(K&& key, Args&&... args) -> std::pair<iterator, bool> {
	// obtain bucket
	if (load_factor() >= 0.8f) { rehash(m_table.size() * 2U); }
	auto const bidx = bucket(key);
	auto* node = &m_table[bidx];

	// find empty node
	while (node->kvp) {
		if (!node->next) { node->next = ktl::make_unique<node_t>(); }
		node = node->next.get();
	}

	// assign
	node->kvp.emplace(value_type{std::forward<K>(key), std::forward<Args>(args)...});
	++m_size.value;
	return {{m_table, bidx, node}, true};
}
} // namespace ktl
