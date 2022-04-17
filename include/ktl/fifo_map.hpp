// KTL header-only library
// Requirements: C++20

#include "hash_table.hpp"
#include <span>

namespace ktl {
///
/// \brief Lightweight FIFO map
///
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class fifo_map {
  public:
	using key_type = Key;
	using mapped_type = Value;
	using value_type = std::pair<Key, Value>;
	using storage_type = std::vector<value_type>;
	using iterator = typename std::span<std::pair<Key const, Value>>::iterator;
	using const_iterator = typename std::span<std::pair<Key const, Value> const>::iterator;
	using reverse_iterator = typename std::span<std::pair<Key const, Value>>::reverse_iterator;
	using const_reverse_iterator = typename std::span<std::pair<Key const, Value> const>::reverse_iterator;

	fifo_map() = default;
	fifo_map(std::initializer_list<value_type> init);
	template <typename InputIt>
	fifo_map(InputIt first, InputIt last);

	template <typename... Args>
	std::pair<iterator, bool> emplace(Key key, Args&&... args);
	std::pair<iterator, bool> insert_or_assign(Key const& key, Value value);
	std::pair<iterator, bool> insert_or_assign(Key&& key, Value value);
	bool erase(Key const& key);
	iterator erase(const_iterator it);

	iterator find(Key const& key);
	bool contains(Key const& key) const { return find(key) != end(); }
	const_iterator find(Key const& key) const;
	mapped_type& operator[](Key const& key);

	std::size_t size() const { return m_storage.size(); }
	bool empty() const { return m_storage.empty(); }
	void clear() noexcept;

	std::span<std::pair<Key const, Value>> span();
	std::span<std::pair<Key const, Value> const> span() const;

	iterator begin() { return span().begin(); }
	iterator end() { return span().end(); }
	const_iterator begin() const { return span().begin(); }
	const_iterator end() const { return span().end(); }
	reverse_iterator rbegin() { return span().rbegin(); }
	reverse_iterator rend() { return span().rend(); }
	const_reverse_iterator rbegin() const { return span().rbegin(); }
	const_reverse_iterator rend() const { return span().rend(); }

  private:
	template <typename K, typename... Args>
	std::pair<iterator, bool> emplace_impl(K&& key, Args&&... args);

	void reindex();

	std::vector<value_type> m_storage{};
	hash_table<Key, std::size_t> m_indices{};
};

template <typename Key, typename Value, typename Hash>
fifo_map<Key, Value, Hash>::fifo_map(std::initializer_list<value_type> init) {
	for (auto const& value : init) { emplace(value.first, value.second); }
}

template <typename Key, typename Value, typename Hash>
template <typename InputIt>
fifo_map<Key, Value, Hash>::fifo_map(InputIt first, InputIt last) {
	for (; first != last; ++first) { emplace(*first); }
}

template <typename Key, typename Value, typename Hash>
template <typename... Args>
auto fifo_map<Key, Value, Hash>::emplace(Key key, Args&&... args) -> std::pair<iterator, bool> {
	if (auto it = find(key); it != end()) { return {it, false}; }
	return emplace_impl(std::move(key), std::forward<Args>(args)...);
}

template <typename Key, typename Value, typename Hash>
auto fifo_map<Key, Value, Hash>::insert_or_assign(Key const& key, Value value) -> std::pair<iterator, bool> {
	if (auto it = find(key); it != end()) {
		it->second = std::move(value);
		return {it, false};
	}
	return emplace_impl(key, std::move(value));
}

template <typename Key, typename Value, typename Hash>
auto fifo_map<Key, Value, Hash>::insert_or_assign(Key&& key, Value value) -> std::pair<iterator, bool> {
	if (auto it = find(key); it != end()) {
		it->second = std::move(value);
		return {it, false};
	}
	return emplace_impl(std::move(key), std::move(value));
}

template <typename Key, typename Value, typename Hash>
bool fifo_map<Key, Value, Hash>::erase(Key const& key) {
	if (auto it = m_indices.find(key); it != m_indices.end()) {
		m_storage.erase(m_storage.begin() + it->second);
		m_indices.erase(it);
		reindex();
		return true;
	}
	return false;
}

template <typename Key, typename Value, typename Hash>
auto fifo_map<Key, Value, Hash>::erase(const_iterator it) -> iterator {
	if (it == end()) { return it; }
	auto const index = std::distance(begin(), it);
	m_storage.erase(m_storage.begin() + index);
	m_indices.erase(index);
	reindex();
	return begin() + index;
}

template <typename Key, typename Value, typename Hash>
auto fifo_map<Key, Value, Hash>::find(Key const& key) -> iterator {
	if (auto it = m_indices.find(key); it != m_indices.end()) { return begin() + it->second; }
	return end();
}

template <typename Key, typename Value, typename Hash>
auto fifo_map<Key, Value, Hash>::find(Key const& key) const -> const_iterator {
	if (auto it = m_indices.find(key); it != m_indices.end()) { return begin() + it->second; }
	return end();
}

template <typename Key, typename Value, typename Hash>
void fifo_map<Key, Value, Hash>::clear() noexcept {
	m_storage.clear();
	m_indices.clear();
}

template <typename Key, typename Value, typename Hash>
std::span<std::pair<Key const, Value>> fifo_map<Key, Value, Hash>::span() {
	if (empty()) { return {}; }
	return std::span{reinterpret_cast<std::pair<Key const, Value>*>(&m_storage.front()), m_storage.size()};
}

template <typename Key, typename Value, typename Hash>
std::span<std::pair<Key const, Value> const> fifo_map<Key, Value, Hash>::span() const {
	if (empty()) { return {}; }
	return std::span{reinterpret_cast<std::pair<Key const, Value> const*>(&m_storage.front()), m_storage.size()};
}

template <typename Key, typename Value, typename Hash>
auto fifo_map<Key, Value, Hash>::operator[](Key const& key) -> mapped_type& {
	auto it = m_indices.find(key);
	if (it == m_indices.end()) { return emplace_impl(key, mapped_type{}).first->second; }
	return m_storage[it->second].second;
}

template <typename Key, typename Value, typename Hash>
template <typename K, typename... Args>
auto fifo_map<Key, Value, Hash>::emplace_impl(K&& key, Args&&... args) -> std::pair<iterator, bool> {
	m_indices.insert_or_assign(key, m_storage.size());
	m_storage.push_back({std::move(key), mapped_type{std::forward<Args...>(args)...}});
	return {end() - 1, true};
}

template <typename Key, typename Value, typename Hash>
void fifo_map<Key, Value, Hash>::reindex() {
	for (std::size_t i = 0; i < m_storage.size(); ++i) { m_indices.insert_or_assign(m_storage[i].first, i); }
}
} // namespace ktl
