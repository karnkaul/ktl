// KTL header-only library
// Requirements: C++20

#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <new>
#include <span>
#include <type_traits>

namespace ktl {
///
/// \brief Fixed-size vector-like container using bytearray as storage
/// Refer to std::vector for API documentation
///
template <typename T, std::size_t Capacity>
class fixed_vector {
	static_assert(!std::is_reference_v<T>, "T must be an object type");
	template <typename U>
	using enable_if_iterator = std::enable_if_t<!std::is_same_v<typename std::iterator_traits<U>::iterator_category, void>>;
	static constexpr bool noexcept_movable_v = std::is_nothrow_move_constructible_v<T>;
	static constexpr bool noexcept_copiable_v = std::is_nothrow_copy_constructible_v<T>;

  public:
	using size_type = std::size_t;
	using value_type = T;

	template <bool IsConst>
	class iter_t;
	using iterator = iter_t<false>;
	using const_iterator = iter_t<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	static constexpr size_type max_size() noexcept { return Capacity; }

	fixed_vector() = default;
	explicit fixed_vector(size_type count, T const& t = T{}) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T>;
	fixed_vector(std::initializer_list<T> init) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T>;
	template <typename InputIt, typename = enable_if_iterator<InputIt>>
	fixed_vector(InputIt first, InputIt last) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T>;

	fixed_vector(fixed_vector&& rhs) noexcept(noexcept_movable_v) : fixed_vector() { exchg(*this, rhs); }
	fixed_vector(fixed_vector const& rhs) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T>;
	fixed_vector& operator=(fixed_vector rhs) noexcept(noexcept_movable_v) { return (exchg(*this, rhs), *this); }
	~fixed_vector() noexcept { clear(); }

	T& at(size_type index) noexcept;
	T const& at(size_type index) const noexcept;
	T& operator[](size_type index) noexcept { return at(index); }
	T const& operator[](size_type index) const noexcept { return at(index); }
	T& front() noexcept { return at(0); }
	T const& front() const noexcept { return at(0); }
	T& back() noexcept { return at(m_size - 1); }
	T const& back() const noexcept { return at(m_size - 1); }
	T* data() noexcept { return empty() ? nullptr : &at(0); }
	T const* data() const noexcept { return empty() ? nullptr : &at(0); }

	iterator begin() noexcept { return iterator(&m_storage, 0); }
	iterator end() noexcept { return iterator(&m_storage, m_size); }
	const_iterator cbegin() const noexcept { return const_iterator(&m_storage, 0); }
	const_iterator cend() const noexcept { return const_iterator(&m_storage, m_size); }
	const_iterator begin() const noexcept { return const_iterator(&m_storage, 0); }
	const_iterator end() const noexcept { return const_iterator(&m_storage, m_size); }
	reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
	reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cbegin()); }
	const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cend()); }

	bool empty() const noexcept { return m_size == 0; }
	size_type size() const noexcept { return m_size; }
	constexpr size_type capacity() const noexcept { return Capacity; }
	bool has_space() const noexcept { return m_size < Capacity; }

	void clear() noexcept;
	iterator insert(const_iterator pos, T const& t) noexcept(noexcept_copiable_v) { return emplace(pos, t); }
	iterator insert(const_iterator pos, T&& t) noexcept(noexcept_movable_v) { return emplace(pos, std::move(t)); }
	iterator insert(const_iterator pos, size_type count, T const& t) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T>;
	template <typename InputIt, typename = enable_if_iterator<InputIt>>
	iterator insert(const_iterator pos, InputIt first, InputIt last) noexcept(noexcept_copiable_v);
	iterator insert(const_iterator pos, std::initializer_list<T> ilist) noexcept(noexcept_copiable_v);
	template <typename... Args>
	iterator emplace(const_iterator pos, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>);
	iterator erase(const_iterator pos) noexcept;
	iterator erase(const_iterator first, const_iterator last) noexcept;
	void push_back(T&& t) noexcept(noexcept_movable_v) { emplace_back(std::move(t)); }
	void push_back(T const& t) noexcept(noexcept_copiable_v) { emplace_back(t); }
	template <typename... Args>
	T& emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>);
	void pop_back() noexcept;
	void resize(size_type count, T const& t = {}) noexcept;

	bool operator==(fixed_vector const& rhs) const noexcept;

	operator std::span<T>() noexcept { return {data(), size()}; }
	operator std::span<T const>() const noexcept { return {data(), size()}; }

  private:
	using storage_t = std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, Capacity>;

	static void exchg(fixed_vector& lhs, fixed_vector& rhs) noexcept(noexcept_movable_v);

	template <typename Ret, typename St>
	static Ret cast(St& st, size_type index) noexcept {
		return std::launder(reinterpret_cast<Ret>(&(st)[index]));
	}

	storage_t m_storage;
	size_type m_size = 0;

	template <bool IsConst>
	friend class iter_t;
};

// impl

template <typename T, std::size_t Capacity>
template <bool IsConst>
class fixed_vector<T, Capacity>::iter_t {
	template <typename U>
	using type_t = std::conditional_t<IsConst, U const, U>;

  public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;

	using pointer = type_t<T>*;
	using reference = type_t<T>&;
	using storage_t = type_t<fixed_vector<T, Capacity>::storage_t>;

	iter_t() = default;
	// Implicit conversion to const iter_t
	operator iter_t<true>() const noexcept { return iter_t<true>(m_storage, m_index); }

	reference operator*() const noexcept { return *acquire(); }
	pointer operator->() const noexcept { return acquire(); }
	reference operator[](size_type index) const noexcept { return *iter_t(m_storage, m_index + index); }

	iter_t& operator++() noexcept { return increment(1U); }
	iter_t& operator--() noexcept { return decrement(1U); }
	iter_t operator++(int) noexcept { return iter_t(m_storage, m_index++); }
	iter_t operator--(int) noexcept { return iter_t(m_storage, m_index--); }
	iter_t& operator+=(difference_type i) noexcept { return increment(cast(i)); }
	iter_t& operator-=(difference_type i) noexcept { return decrement(cast(i)); }
	iter_t operator+(difference_type i) const noexcept { return iter_t(m_storage, m_index + cast(i)); }
	iter_t operator-(difference_type i) const noexcept { return iter_t(m_storage, m_index - cast(i)); }
	difference_type operator+(iter_t const& rhs) const noexcept { return cast(m_index) + cast(rhs.m_index); }
	difference_type operator-(iter_t const& rhs) const noexcept { return cast(m_index) - cast(rhs.m_index); }

	bool operator==(iter_t const& rhs) const noexcept { return m_storage == rhs.m_storage && m_index == rhs.m_index; }
	std::partial_ordering operator<=>(iter_t const& rhs) const noexcept {
		if (m_storage != rhs.m_storage) { return std::partial_ordering::unordered; }
		return m_index <=> rhs.m_index;
	}

  private:
	constexpr static difference_type cast(size_type s) noexcept { return static_cast<difference_type>(s); }
	constexpr static size_type cast(difference_type d) noexcept { return static_cast<size_type>(d); }

	iter_t(storage_t* storage, size_type index) noexcept : m_storage(storage), m_index(index) {}

	pointer acquire() const noexcept { return m_ptr ? m_ptr : m_ptr = fixed_vector::cast<pointer>(*m_storage, m_index); }
	iter_t& increment(size_type delta) noexcept { return (m_index += delta, release()); }
	iter_t& decrement(size_type delta) noexcept { return (m_index -= delta, release()); }
	iter_t& release() noexcept {
		m_ptr = {};
		return *this;
	}

	storage_t* m_storage{};
	size_type m_index{};
	mutable pointer m_ptr{};

	friend class fixed_vector<T, Capacity>;
};

template <typename T, std::size_t Capacity>
fixed_vector<T, Capacity>::fixed_vector(size_type count, T const& t) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T> {
	assert(count <= capacity());
	for (size_type i = 0; i < count; ++i) { push_back(t); }
}
template <typename T, std::size_t Capacity>
fixed_vector<T, Capacity>::fixed_vector(std::initializer_list<T> init) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T> {
	assert(init.size() <= capacity());
	for (T const& t : init) { push_back(t); }
}
template <typename T, std::size_t Capacity>
template <typename InputIt, typename>
fixed_vector<T, Capacity>::fixed_vector(InputIt first, InputIt last) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T> {
	for (; first != last; ++first) { push_back(*first); }
}
template <typename T, std::size_t Capacity>
fixed_vector<T, Capacity>::fixed_vector(fixed_vector const& rhs) noexcept(noexcept_copiable_v) requires std::is_copy_constructible_v<T> {
	if constexpr (std::is_trivial_v<T>) {
		std::memcpy(m_storage.data(), rhs.m_storage.data(), rhs.size() * sizeof(T));
		m_size = rhs.m_size;
	} else {
		for (T const& t : rhs) { push_back(t); }
	}
}
template <typename T, std::size_t Capacity>
void fixed_vector<T, Capacity>::exchg(fixed_vector& lhs, fixed_vector& rhs) noexcept(noexcept_movable_v) {
	if constexpr (std::is_trivial_v<T>) {
		std::swap(lhs.m_storage, rhs.m_storage);
		std::swap(lhs.m_size, rhs.m_size);
	} else {
		fixed_vector tmp;
		for (auto& t : lhs) { tmp.push_back(std::move(t)); }
		lhs.clear();
		for (auto& t : rhs) { lhs.push_back(std::move(t)); }
		rhs.clear();
		for (auto& t : tmp) { rhs.push_back(std::move(t)); }
	}
}
template <typename T, std::size_t Capacity>
T& fixed_vector<T, Capacity>::at(size_type index) noexcept {
	assert(index < size());
	return *cast<T*>(m_storage, index);
}
template <typename T, std::size_t Capacity>
T const& fixed_vector<T, Capacity>::at(size_type index) const noexcept {
	assert(index < size());
	return *cast<T const*>(m_storage, index);
}
template <typename T, std::size_t Capacity>
void fixed_vector<T, Capacity>::clear() noexcept {
	if constexpr (std::is_trivial_v<T>) {
		m_size = 0;
	} else {
		while (!empty()) { pop_back(); }
	}
}
template <typename T, std::size_t Capacity>
auto fixed_vector<T, Capacity>::insert(const_iterator pos, size_type count, T const& t) noexcept(noexcept_copiable_v)
	-> iterator requires std::is_copy_constructible_v<T> {
	if (count == 0) { return pos; }
	size_type const ret = pos.m_index;
	for (; count > 0; --count) { pos = emplace(pos, t); }
	return iterator(&m_storage, ret);
}
template <typename T, std::size_t Capacity>
template <typename InputIt, typename>
auto fixed_vector<T, Capacity>::insert(const_iterator pos, InputIt first, InputIt last) noexcept(noexcept_copiable_v) -> iterator {
	if (std::distance(first, last) == 0) { return pos; }
	size_type const ret = pos.m_index;
	for (; first != last; ++first) { pos = emplace(pos, *first); }
	return iterator(&m_storage, ret);
}
template <typename T, std::size_t Capacity>
auto fixed_vector<T, Capacity>::insert(const_iterator pos, std::initializer_list<T> ilist) noexcept(noexcept_copiable_v) -> iterator {
	return insert(pos, ilist.begin(), ilist.end());
}
template <typename T, std::size_t Capacity>
template <typename... Args>
auto fixed_vector<T, Capacity>::emplace(const_iterator pos, Args&&... u) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> iterator {
	assert(has_space());
	if (pos == end()) {
		emplace_back(std::forward<Args>(u)...);
		return iterator(&m_storage, m_size - 1);
	}
	using std::swap;
	size_type idx = pos.m_index;
	T temp{};
	for (; idx < m_size; ++idx) { swap(temp, at(idx)); }
	emplace_back(std::move(temp));
	at(pos.m_index) = T{std::forward<Args>(u)...};
	return iterator(&m_storage, pos.m_index);
}
template <typename T, std::size_t Capacity>
auto fixed_vector<T, Capacity>::erase(const_iterator pos) noexcept -> iterator {
	for (size_type idx = pos.m_index; idx < m_size - 1; ++idx) { at(idx) = std::move(at(idx + 1)); }
	pop_back();
	return iterator(&m_storage, pos.m_index);
}
template <typename T, std::size_t Capacity>
auto fixed_vector<T, Capacity>::erase(const_iterator first, const_iterator last) noexcept -> iterator {
	auto const first_idx = first.m_index;
	if (last.m_index - first_idx == 0) { return iterator(&m_storage, last.m_index); }
	// shift range to end by moving end to middle
	while (last.m_index < m_size) { at(first.m_index++) = std::move(at(last.m_index++)); }
	// pop back till range is empty
	while (m_size > first.m_index) { pop_back(); }
	return iterator(&m_storage, first_idx);
}
template <typename T, std::size_t Capacity>
template <typename... Args>
T& fixed_vector<T, Capacity>::emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
	assert(has_space());
	T* t = new (&m_storage[m_size]) T(std::forward<Args>(args)...);
	++m_size;
	return *t;
}
template <typename T, std::size_t Capacity>
void fixed_vector<T, Capacity>::pop_back() noexcept {
	assert(!empty());
	if constexpr (!std::is_trivial_v<T>) {
		T* t = &back();
		t->~T();
	}
	--m_size;
}
template <typename T, std::size_t Capacity>
void fixed_vector<T, Capacity>::resize(size_type count, T const& t) noexcept {
	while (m_size > count) { pop_back(); }
	while (count > m_size) { push_back(t); }
}
template <typename T, std::size_t Capacity>
bool fixed_vector<T, Capacity>::operator==(fixed_vector<T, Capacity> const& rhs) const noexcept {
	if (size() != rhs.size()) { return false; }
	for (typename fixed_vector<T, Capacity>::size_type i = 0; i < size(); ++i) {
		if ((*this)[i] != rhs[i]) { return false; }
	}
	return true;
}
} // namespace ktl
