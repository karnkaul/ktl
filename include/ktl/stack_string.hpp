// KTL header-only library
// Requirements: C++20

#pragma once
#include "str_format.hpp"

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

///
/// \brief Stream buffer view that uses a fixed-range buffer
///
struct streambuf_view : std::streambuf {
	template <std::size_t Size>
	explicit streambuf_view(char (&buf)[Size]) noexcept : streambuf_view(buf, Size) {}
	streambuf_view(char* begin, std::size_t extent) noexcept { setp(begin, begin + extent); }
};

template <std::size_t Capacity>
	requires(Capacity > 0)
class stack_string {
  public:
	inline static constexpr std::size_t npos = std::string::npos;
	inline static constexpr std::size_t capacity_v = Capacity;

	constexpr stack_string(std::string_view str = {}) noexcept { write(str); }
	template <std::size_t N>
	constexpr stack_string(char const (&arr)[N]) noexcept;
	constexpr stack_string(std::string const& str) noexcept { write(str); }
	template <ostreamable Arg, ostreamable... Args>
	stack_string(std::string_view fmt, Arg const& arg, Args const&... args) noexcept;

	template <std::size_t N>
	constexpr stack_string(stack_string<N> const& rhs) noexcept : stack_string(rhs.get()) {}
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
	constexpr void write(std::string_view str) noexcept;
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
	write(arr);
}

template <std::size_t Capacity>
	requires(Capacity > 0)
template <ostreamable Arg, ostreamable... Args>
stack_string<Capacity>::stack_string(std::string_view fmt, Arg const& arg, Args const&... args) noexcept {
	streambuf_view buf(m_str);
	std::iostream stream(&buf);
	detail::format_str<str_formatter>(stream, fmt, arg, args...);
	term(len(m_str));
}

template <std::size_t Capacity>
	requires(Capacity > 0)
template <std::size_t N>
constexpr stack_string<Capacity>& stack_string<Capacity>::operator+=(stack_string<N> const& rhs) noexcept {
	write(rhs);
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
constexpr void stack_string<Capacity>::write(std::string_view str) noexcept {
	std::size_t i = 0U;
	for (; i < str.size() && m_extent + i + 1U < Capacity; ++i) { m_str[m_extent + i] = str[i]; }
	term(m_extent + i);
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
