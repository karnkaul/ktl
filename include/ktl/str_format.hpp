// KTL header-only library
// Requirements: C++20

#pragma once
#include "str_formatter.hpp"
#include <sstream>

namespace ktl {
///
/// \brief Format string via fmt and args... into out
///
template <typename Formatter = string_formatter<>, typename... Args>
std::ostream& str_format(std::ostream& out, std::string_view fmt, Args const&... args) {
	return Formatter{}(out, fmt, args...);
}

///
/// \brief Obtain a std::string formatted using fmt and args...
///
template <typename Formatter = string_formatter<>, typename... Args>
std::string str_format(std::string_view fmt, Args const&... args) {
	std::stringstream str;
	str_format(str, fmt, args...);
	return str.str();
}
} // namespace ktl
