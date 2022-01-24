// KTL header-only library
// Requirements: C++20

#include <cassert>
#include <concepts>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>

namespace ktl {
template <typename T>
concept ostreamable = requires(std::ostream& o, T const& t) {
	{ o << t } -> std::convertible_to<std::ostream&>;
};

///
/// \brief Customization point: interpolates passed argument using passed format specifier string
///
struct string_interpolator {
	///
	/// \brief Write t into out using fmt as the format specifier (non-empty)
	///
	template <ostreamable T>
	void operator()(std::ostream& out, std::string_view fmt, T const& t) const;
};

///
/// \brief String formatter using argument interpolator
///
template <typename Interpolator = string_interpolator, char Begin = '{', char End = '}'>
class string_formatter;

// impl

namespace detail {
struct str_format_token {
	enum class kind_t { string, argument, eof };
	std::string_view str;
	kind_t kind = kind_t::eof;

	explicit constexpr operator bool() const noexcept { return kind != kind_t::eof; }
};

template <char Begin, char End>
struct str_format_scanner {
	struct entry {
		str_format_token token;
		std::size_t next{};
	};

	std::string_view text;
	entry current;

	constexpr entry peek() const noexcept;
	constexpr str_format_token next() noexcept;
};
} // namespace detail

template <typename Interpolator, char Begin, char End>
class string_formatter {
  public:
	template <ostreamable... Args>
	std::ostream& operator()(std::ostream& out, std::string_view fmt, Args const&... args);

  private:
	template <typename T, typename... Ts>
	std::ostream& format(std::ostream& out, T const& t, Ts const&... ts);
	template <typename T>
	static void interpolate(std::ostream& out, std::string_view fmt, T const& t);

	detail::str_format_scanner<Begin, End> m_scanner;
};

template <ostreamable T>
void string_interpolator::operator()(std::ostream& out, std::string_view fmt, T const& t) const {
	if constexpr (std::is_trivial_v<T>) {
		static constexpr std::size_t buf_size = 128;
		static constexpr std::size_t fmt_size = 8;
		char buf[buf_size] = {};
		char meta[fmt_size] = "%";
		assert(fmt.size() < fmt_size);
		std::memcpy(meta + 1, fmt.data(), fmt.size());
		std::snprintf(buf, buf_size, meta, t);
		out << buf;
	} else {
		out << t;
	}
}

template <char Begin, char End>
constexpr auto detail::str_format_scanner<Begin, End>::peek() const noexcept -> entry {
	if (current.next >= text.size()) { return {}; }
	entry ret;
	if (text[current.next] == Begin) {
		// extract argument
		ret.token.kind = str_format_token::kind_t::argument;
		ret.next = text.find(End, current.next);
		if (ret.next == std::string_view::npos || ret.next <= current.next) { return {}; }
		ret.token.str = text.substr(current.next + 1U, ret.next - current.next - 1U);
		++ret.next;
		return ret;
	}
	// interpolate to next argument / eof
	ret.token.kind = str_format_token::kind_t::string;
	ret.next = text.find(Begin, current.next);
	ret.token.str = ret.next == std::string_view::npos ? text.substr(current.next) : text.substr(current.next, ret.next - current.next);
	return ret;
}

template <char Begin, char End>
constexpr auto detail::str_format_scanner<Begin, End>::next() noexcept -> str_format_token {
	current = peek();
	return current.token;
}

template <typename Interpolator, char Begin, char End>
template <ostreamable... Args>
std::ostream& string_formatter<Interpolator, Begin, End>::operator()(std::ostream& out, std::string_view fmt, Args const&... args) {
	if constexpr (sizeof...(Args) == 0) { return out << fmt; }
	m_scanner.text = fmt;
	return format(out, args...);
}

template <typename Interpolator, char Begin, char End>
template <typename T, typename... Ts>
std::ostream& string_formatter<Interpolator, Begin, End>::format(std::ostream& out, T const& t, Ts const&... ts) {
	while (detail::str_format_token const token = m_scanner.next()) {
		switch (token.kind) {
		case detail::str_format_token::kind_t::argument: {
			interpolate(out, token.str, t);									// interpolate t
			if constexpr (sizeof...(Ts) > 0) { return format(out, ts...); } // recurse into remaining ts
			break;
		}
		case detail::str_format_token::kind_t::string: out << token.str; break; // write string
		default: break;
		}
	}
	return out;
}

template <typename Interpolator, char Begin, char End>
template <typename T>
void string_formatter<Interpolator, Begin, End>::interpolate(std::ostream& out, std::string_view fmt, T const& t) {
	if (fmt.empty()) {
		out << t; // no format string, write argument directly
	} else {
		Interpolator{}(out, fmt, t); // interpolate using fmt
	}
}
} // namespace ktl
