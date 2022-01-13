// KTL header-only library
// Requirements: C++20

#pragma once
#include <cassert>
#include <iterator>
#include <span>
#include <vector>
#include "ring_counter.hpp"

namespace ktl {
template <typename T>
concept RingElement = std::is_default_constructible_v<T> && !std::is_reference_v<T>;

///
/// \brief Rotating ring-buffer using contiguous storage. Overwrites on overflow.
///
template <RingElement T, typename Storage = std::vector<T>>
class ring_buffer {
	template <bool Const>
	class iter_t;

  public:
	using value_type = T;
	using storage_t = Storage;

	using iterator = iter_t<false>;
	using const_iterator = iter_t<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	constexpr ring_buffer(std::size_t capacity) noexcept : m_storage(capacity + 1U, T{}), m_read(std::size(m_storage)), m_write(std::size(m_storage)) {}
	constexpr ring_buffer(Storage storage) noexcept : m_storage(std::move(storage)), m_read(std::size(m_storage)), m_write(std::size(m_storage)) {}

	constexpr bool push(T t) noexcept(std::is_nothrow_move_assignable_v<T>);
	constexpr void pop();
	constexpr std::size_t capacity() const noexcept { return std::empty(m_storage) ? 0U : std::size(m_storage) - 1U; }
	constexpr std::size_t size() const noexcept { return m_write - m_read; }
	constexpr bool empty() const noexcept { return m_write == m_read; }
	constexpr T const& back() const noexcept { return (assert(!empty()), m_storage[m_read]); }
	constexpr T& back() noexcept { return (assert(!empty()), m_storage[m_read]); }
	constexpr void clear() noexcept;

	constexpr iterator begin() noexcept { return {make_span(), m_read}; }
	constexpr iterator end() noexcept { return {make_span(), m_write}; }
	constexpr const_iterator begin() const noexcept { return {make_span(), m_read}; }
	constexpr const_iterator end() const noexcept { return {make_span(), m_write}; }
	constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
	constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  private:
	constexpr std::span<T> make_span() noexcept { return std::span(std::data(m_storage), std::size(m_storage)); }
	constexpr std::span<T const> make_span() const noexcept { return std::span(std::data(m_storage), std::size(m_storage)); }

	Storage m_storage{};
	ring_index m_read;
	ring_index m_write;
};

template <RingElement T, typename Storage>
template <bool IsConst>
class ring_buffer<T, Storage>::iter_t {
	template <typename U>
	using type_t = std::conditional_t<IsConst, U const, U>;

  public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = type_t<T>*;
	using reference = type_t<T>&;

	constexpr iter_t() = default;
	// Implicit conversion to const iter_t
	operator iter_t<true>() const noexcept { return iter_t<true>(m_storage, m_index); }

	constexpr bool operator==(iter_t const& rhs) const noexcept { return m_storage.begin() == rhs.m_storage.begin() && m_index == rhs.m_index; }
	constexpr iter_t& operator++() noexcept { return (++m_index, *this); }
	constexpr iter_t operator++(int) noexcept {
		auto ret = *this;
		++(*this);
		return ret;
	}
	constexpr iter_t& operator--() noexcept { return (--m_index, *this); }
	constexpr iter_t operator--(int) noexcept {
		auto ret = *this;
		--(*this);
		return ret;
	}
	constexpr pointer operator->() const noexcept { return std::addressof(m_storage[m_index]); }
	constexpr reference operator*() const noexcept { return m_storage[m_index]; }

  private:
	constexpr iter_t(std::span<T> storage, ring_index index) noexcept : m_storage(storage), m_index(index) {}
	std::span<T> m_storage{};
	ring_index m_index;
	friend class ring_buffer;
};

template <RingElement T>
ring_buffer(std::vector<T>) -> ring_buffer<T, std::vector<T>>;

template <RingElement T, std::size_t N>
ring_buffer(std::array<T, N>) -> ring_buffer<T, std::array<T, N>>;

// impl

template <RingElement T, typename Storage>
constexpr bool ring_buffer<T, Storage>::push(T t) noexcept(std::is_nothrow_move_assignable_v<T>) {
	m_storage[m_write++] = std::move(t);
	if (m_write == m_read) {
		++m_read;
		return false;
	}
	return true;
}

template <RingElement T, typename Storage>
constexpr void ring_buffer<T, Storage>::pop() {
	assert(!empty());
	m_storage[m_read++] = {};
}

template <RingElement T, typename Storage>
constexpr void ring_buffer<T, Storage>::clear() noexcept {
	while (!empty()) { m_storage[m_write++] = {}; }
}
} // namespace ktl
