// KTL header-only library
// Requirements: C++20

#pragma once
#include <forward_list>
#include <type_traits>

namespace ktl {
///
/// \brief Models a "forward" N-tree (no parent link) via std::forward_list
///
template <typename T>
	requires(!std::is_reference_v<T>)
class n_tree {
  public:
	using value_type = T;
	using storage_t = std::forward_list<n_tree>;

	explicit n_tree(T t = T{}) noexcept : m_t(std::move(t)) {}

	///
	/// \brief Add t to front of children
	///
	n_tree& push_front(T t) { return (m_children.emplace_front(std::move(t)), *m_children.begin()); }
	///
	/// \brief Destroy all children
	///
	void clear_children() noexcept { m_children.clear(); }
	///
	/// \brief Erase node if child (recursive)
	///
	bool erase_child(n_tree const& node) noexcept;

	///
	/// \brief Check if this node has any child nodes
	///
	bool has_children() const noexcept { return !m_children.empty(); }
	///
	/// \brief Obtain (a const reference to) all child nodes
	///
	storage_t const& children() const noexcept { return m_children; }

	///
	/// \brief Perform a DFS using a predicate (called with T passed)
	///
	template <typename Pred>
	n_tree* depth_first_find(Pred pred) noexcept;
	///
	/// \brief Perform a DFS using a predicate (called with T passed)
	///
	template <typename Pred>
	n_tree const* depth_first_find(Pred pred) const noexcept;

	///
	/// \brief Payload
	///
	T m_t;

  private:
	storage_t m_children;
};

// impl

template <typename T>
	requires(!std::is_reference_v<T>)
bool n_tree<T>::erase_child(n_tree const& node) noexcept {
	if (!has_children()) { return false; }
	// First node is a special case (cannot erase_after())
	auto it = m_children.begin();
	if (&*it == &node) {
		m_children.pop_front();
		return true;
	}
	// Depth first
	if (it->erase_child(node)) { return true; }
	// Other nodes are general cases
	for (auto prev = it++; it != m_children.end(); ++it) {
		if (&*it == &node) {
			m_children.erase_after(prev);
			return true;
		}
		// Depth first
		if (it->erase_child(node)) { return true; }
		prev = it;
	}
	return false;
}
template <typename T>
	requires(!std::is_reference_v<T>)
template <typename Pred>
n_tree<T>* n_tree<T>::depth_first_find(Pred pred) noexcept {
	for (auto& child : m_children) {
		if (auto ret = child.find(pred)) { return ret; }
	}
	if (pred(m_t)) { return this; }
	return nullptr;
}
template <typename T>
	requires(!std::is_reference_v<T>)
template <typename Pred>
n_tree<T> const* n_tree<T>::depth_first_find(Pred pred) const noexcept {
	for (auto const& child : m_children) {
		if (auto const ret = child.find(pred)) { return ret; }
	}
	if (pred(m_t)) { return this; }
	return nullptr;
}
} // namespace ktl
