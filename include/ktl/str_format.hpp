// KTL header-only library
// Requirements: C++20

#pragma once
#include <cassert>
#include <concepts>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string_view>

namespace ktl {
template <typename T>
concept ostreamable = requires(std::ostream& o, T const& t) {
	{ o << t } -> std::convertible_to<std::ostream&>;
};

///
/// \brief Default formatter
/// Interpolation token: {X} where X is an optional C style format specifier without the leading % (eg {.3f}, {x})
/// Format spec: all specifiers that are std::printf compatible
///
struct str_formatter;

///
/// \brief Format using Ftr and return a std::string
///
template <typename Ftr = str_formatter, ostreamable... Args>
std::string format(std::string_view fmt, Args const&... args);

// impl

namespace detail {
template <typename Ftr, ostreamable T, ostreamable... Ts>
std::ostream& format_str(std::ostream& out, std::string_view fmt, T const& t, Ts const&... ts) {
	std::size_t const bb = fmt.find(Ftr::begin_v);
	std::size_t const be = fmt.find(Ftr::end_v);
	if (bb < fmt.size() && be < fmt.size() && bb < be) {
		out << fmt.substr(0, bb);
		Ftr{}(out, fmt.substr(bb, be - bb), t);
		auto const residue = fmt.substr(be + 1);
		if constexpr (sizeof...(Ts) > 0) {
			return format_str<Ftr>(out, residue, ts...);
		} else {
			return out << residue;
		}
	}
	return out << fmt;
}
} // namespace detail

struct str_formatter {
	static constexpr char begin_v = '{';
	static constexpr char end_v = '}';

	template <ostreamable T>
	std::ostream& operator()(std::ostream& out, std::string_view fmt_spec, T const& t) {
		if constexpr (std::is_trivial_v<T>) {
			if (fmt_spec.size() > 2) {
				static constexpr std::size_t buf_size = 128;
				static constexpr std::size_t fmt_size = 8;
				char buf[buf_size] = {};
				char meta[fmt_size] = "%";
				assert(fmt_spec.size() < fmt_size);
				std::memcpy(meta + 1, fmt_spec.data() + 1, fmt_spec.size() - 1);
				std::snprintf(buf, buf_size, meta, t);
				return out << buf;
			} else {
				return out << t;
			}
		} else {
			return out << t;
		}
	}
};

template <typename Ftr, ostreamable... Args>
std::string format(std::string_view fmt, Args const&... args) {
	std::stringstream str;
	if constexpr (sizeof...(Args) > 0) {
		detail::format_str<Ftr>(str, fmt, args...);
	} else {
		str << fmt;
	}
	return str.str();
}
} // namespace ktl
