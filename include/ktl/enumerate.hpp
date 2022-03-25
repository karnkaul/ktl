// KTL header-only library
// Requirements: C++20

#pragma once
#include <concepts>
#include <iterator>
#include <utility>

namespace ktl {
///
/// \brief Range represented by a pair of iterators and a monotonically increasing index
///
template <typename Iterator, std::integral Index = std::size_t>
class indexed_range;

template <typename Index = std::size_t, typename Iterator>
constexpr auto enumerate(Iterator first, Iterator last) {
	return indexed_range<Iterator, Index>(first, last);
}

template <typename Index = std::size_t, typename Container>
constexpr auto enumerate(Container&& container) {
	return enumerate<Index>(std::begin(container), std::end(container));
}

// impl

template <typename It, std::integral Index>
class indexed_range {
  public:
	using index_type = Index;

	struct iterator;
	using const_iterator = iterator;

	constexpr indexed_range(It first, It last) noexcept : m_begin(first), m_end(last), m_size(static_cast<std::size_t>(std::distance(m_begin, m_end))) {}

	constexpr iterator begin() const { return {m_begin, Index{}}; }
	constexpr iterator end() const { return {m_end, static_cast<Index>(m_size)}; }
	constexpr std::size_t size() const { return m_size; }

  private:
	It m_begin;
	It m_end;
	std::size_t m_size;
};

template <typename It, std::integral Index>
struct indexed_range<It, Index>::iterator {
	using value_type = std::pair<typename std::iterator_traits<It>::reference, Index>;
	using reference = value_type;

	It m_it{};
	Index m_index{};

	constexpr reference operator*() const { return {*m_it, m_index}; }

	constexpr iterator& operator++() { return (++m_it, ++m_index, *this); }
	constexpr iterator operator++(int) {
		auto ret = *this;
		++(*this);
		return ret;
	}

	bool operator==(iterator const& rhs) const = default;
};
} // namespace ktl
