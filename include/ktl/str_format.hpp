// KTL header-only library
// Requirements: C++17

#pragma once
#include <ostream>
#include <sstream>
#include <string_view>

namespace ktl {
///
/// \brief Customization point; metafunction must provide static std::basic_string_view<Ch> value
///
template <typename Ch>
struct fmt_token_t;

///
/// \brief Format a std::string using provided interpolation token ("{}" by default)
///
template <typename Tk = fmt_token_t<char>, typename... Args>
std::string format(std::string_view fmt, Args const&... args);
///
/// \brief Format a std::wstring using provided interpolation token (L"{}" by default)
///
template <typename Tk = fmt_token_t<wchar_t>, typename... Args>
std::wstring format(std::wstring_view fmt, Args const&... args);
///
/// \brief Format a std::basic_string<Ch> using provided interpolation token
///
template <typename Ch = char, typename Tk = fmt_token_t<Ch>, typename... Args>
std::basic_string<Ch> format(std::basic_string_view<Ch> fmt, Args const&... args);

// impl

namespace detail {
template <typename Ch>
std::basic_ostream<Ch>& format_str(std::basic_ostream<Ch>& out, std::basic_string_view<Ch> fmt) {
	return out << fmt;
}

template <typename Ch, typename Tk, typename Arg, typename... Args>
std::basic_ostream<Ch>& format_str(std::basic_ostream<Ch>& out, std::basic_string_view<Ch> fmt, Arg const& arg, Args const&... args) {
	if (auto search = fmt.find(Tk::value); search != std::basic_string<Ch>::npos) {
		std::basic_string_view<Ch> text(fmt.data(), search);
		out << text << arg;
		return format_str(out, fmt.substr(search + Tk::value.size()), args...);
	}
	return format_str(out, fmt);
}
} // namespace detail

template <>
struct fmt_token_t<char> {
	static constexpr std::basic_string_view<char> value = "{}";
};

template <>
struct fmt_token_t<wchar_t> {
	static constexpr std::basic_string_view<wchar_t> value = L"{}";
};

template <typename Tk, typename... Args>
std::string format(std::string_view fmt, Args const&... args) {
	return format<char, Tk>(fmt, args...);
}

template <typename Tk, typename... Args>
std::wstring format(std::wstring_view fmt, Args const&... args) {
	return format<wchar_t, Tk>(fmt, args...);
}

template <typename Ch, typename Tk, typename... Args>
std::basic_string<Ch> format(std::basic_string_view<Ch> fmt, Args const&... args) {
	std::basic_stringstream<Ch> str;
	detail::format_str<Ch, Tk>(str, fmt, args...);
	return str.str();
}
} // namespace ktl
