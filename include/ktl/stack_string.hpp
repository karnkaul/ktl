// KTL header-only library
// Requirements: C++20

#pragma once
#include <cassert>
#include <cstring>
#include <string>

namespace ktl {
///
/// \brief Wrapper for stack allocated char buffer (null terminated)
///
template <std::size_t Capacity>
	requires(Capacity > 0)
class stack_string;

namespace literals {
constexpr stack_string<64> operator""_ss(char const* str, std::size_t size) noexcept;
}

template <std::size_t Capacity>
	requires(Capacity > 0)
class stack_string {

  public:
	inline static constexpr std::size_t npos = std::string::npos;
	inline static constexpr std::size_t capacity_v = Capacity;

	constexpr stack_string() = default;

	template <std::size_t N>
	constexpr stack_string(char const (&arr)[N]) noexcept;
	template <typename... Args>
	constexpr stack_string(std::string_view fmt, Args const&... args) noexcept;
	stack_string(std::string const& str) noexcept : stack_string(str.data(), str.size()) {}

	template <std::size_t N>
	constexpr stack_string(stack_string<N> const& rhs) noexcept : stack_string(rhs.data(), rhs.size()) {}
	template <std::size_t N>
	constexpr stack_string& operator+=(stack_string<N> const& rhs) noexcept;

	constexpr std::size_t capacity() const noexcept { return capacity_v; }
	constexpr std::size_t size() const noexcept { return m_extent; }
	constexpr std::size_t vacant() const noexcept { return capacity() - size(); }
	constexpr char const* data() const noexcept { return m_str; }
	constexpr char* c_str() noexcept { return m_str; }
	constexpr std::string_view get() const noexcept { return data(); }
	constexpr operator std::string_view() const noexcept { return get(); }

  private:
	static constexpr std::size_t len(char const* str) noexcept;
	constexpr void term(std::size_t end) noexcept;

	char m_str[Capacity] = {};
	std::size_t m_extent{};
};

template <std::size_t Capacity>
constexpr stack_string<Capacity> operator+(stack_string<Capacity> const& lhs, stack_string<Capacity> const& rhs) noexcept;

template <std::size_t Capacity>
	requires(Capacity > 0)
template <std::size_t N>
constexpr stack_string<Capacity>::stack_string(char const (&arr)[N]) noexcept {
	std::size_t i = 0;
	for (; i < N && i < Capacity - 1; ++i) { m_str[i] = arr[i]; }
	term(i);
}

template <std::size_t Capacity>
	requires(Capacity > 0)
template <typename... Args>
constexpr stack_string<Capacity>::stack_string(std::string_view fmt, Args const&... args) noexcept {
	if constexpr (sizeof...(Args) == 0) {
		std::size_t i = 0;
		for (; i < fmt.size() && i < Capacity - 1; ++i) { m_str[i] = fmt[i]; }
		term(i);
	} else {
		int const written = std::snprintf(m_str, Capacity, fmt.data(), args...);
		if (written > 0) { m_extent = written >= int(Capacity) ? Capacity - 1 : static_cast<std::size_t>(written); }
	}
}

template <std::size_t Capacity>
	requires(Capacity > 0)
template <std::size_t N>
constexpr stack_string<Capacity>& stack_string<Capacity>::operator+=(stack_string<N> const& rhs) noexcept {
	char* start = m_str + m_extent;
	std::size_t const max_extent = Capacity - m_extent - 1;
	std::size_t const extent = rhs.size() < max_extent ? rhs.size() : max_extent;
	std::memcpy(start, rhs.data(), extent);
	m_extent += extent;
	return *this;
}

template <std::size_t Capacity>
	requires(Capacity > 0)
constexpr std::size_t stack_string<Capacity>::len(char const* str) noexcept {
	std::size_t ret{};
	for (; str && *str; ++str) { ++ret; }
	return ret;
}

template <std::size_t Capacity>
	requires(Capacity > 0)
constexpr void stack_string<Capacity>::term(std::size_t end) noexcept {
	assert(end < Capacity);
	m_extent = end;
	m_str[m_extent] = '\0';
}

template <std::size_t Capacity>
	requires(Capacity > 0)
constexpr stack_string<Capacity> operator+(stack_string<Capacity> const& lhs, stack_string<Capacity> const& rhs) noexcept {
	auto ret = lhs;
	return ret += rhs;
}

namespace literals {
constexpr stack_string<64> operator""_ss(char const* str, std::size_t size) noexcept { return std::string_view{str, size}; }
} // namespace literals
} // namespace ktl
