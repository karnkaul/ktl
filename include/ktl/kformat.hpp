// KTL single-header library
// Requirements: C++20

#pragma once
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

namespace ktl {
///
/// \brief Obtain formatted string
///
template <typename... Args>
[[nodiscard]] std::string kformat(std::string_view const fmt, Args const&... args);

///
/// \brief Interpolate arguments to output string
///
template <typename... Args>
void kformat_to(std::string& out, std::string_view const fmt, Args const&... args);

///
/// \brief Customization point
///
template <typename T>
struct kformatter {
	void operator()(std::string& out, std::string_view fmt, T const& arg) const;
};

// impl

namespace detail {
struct format_arg {
	using do_format_t = void (*)(std::string& out, std::string_view fmt, void const* arg);

	do_format_t do_format{&null_format};
	void const* arg{};

	static constexpr void null_format(std::string&, std::string_view, void const*) {}

	template <typename T>
	static constexpr do_format_t make_do_format() {
		return [](std::string& out, std::string_view fmt, void const* arg) { kformatter<T>{}(out, fmt, *static_cast<T const*>(arg)); };
	}

	template <typename T>
	static constexpr format_arg make(T const& t) {
		return {make_do_format<T>(), &t};
	}
};

template <std::size_t Capacity>
struct format_args {
	format_arg args[Capacity]{};
	std::size_t size{};
	std::size_t index{};

	template <typename... Args>
	static constexpr format_args make(Args const&... args) {
		static_assert(sizeof...(Args) <= Capacity);
		return format_args{{format_arg::make(args)...}, sizeof...(Args)};
	}

	constexpr format_arg next() {
		assert(index < Capacity);
		return args[index++];
	}
};

template <typename T>
std::string kformat_to_string(T const& t) {
	if constexpr (std::convertible_to<T, std::string_view>) {
		return std::string(t);
	} else if constexpr (std::same_as<T, char>) {
		return std::string(1, static_cast<char>(t));
	} else if constexpr (std::is_pointer_v<T>) {
		return std::to_string(reinterpret_cast<std::size_t>(t));
	} else {
		using std::to_string;
		return to_string(t);
	}
}

struct kfmt {
	template <std::size_t Size>
	void operator()(std::string& out, std::string_view fmt, format_args<Size> args) const {
		out.reserve(out.size() + fmt.size() + args.size * 8);
		auto lbrace = fmt.find('{');
		while (lbrace != std::string_view::npos) {
			auto const rbrace = fmt.find('}', lbrace);
			auto const argfmt = fmt.substr(lbrace + 1, rbrace - lbrace - 1);
			auto const arg = args.next();
			out += fmt.substr(0, lbrace);
			arg.do_format(out, argfmt, arg.arg);
			fmt = fmt.substr(rbrace + 1);
			lbrace = fmt.find('{');
		}
		if (!fmt.empty()) { out += std::string(fmt); }
	}

	static constexpr std::size_t size(std::size_t count) {
		constexpr std::size_t min_args_v{16};
		return count < min_args_v ? min_args_v : count;
	}
};
} // namespace detail

template <typename T>
void kformatter<T>::operator()(std::string& out, std::string_view fmt, T const& arg) const {
	if constexpr (std::integral<T> || std::floating_point<T> || std::is_pointer_v<T>) {
		if (fmt.empty() || fmt.size() + 1 >= 16) {
			out += detail::kformat_to_string(arg);
		} else if (fmt[0] == ':') {
			fmt = fmt.substr(1);
			char szfmt[16]{};
			char szbuf[64]{};
			szfmt[0] = '%';
			std::memcpy(szfmt + 1, fmt.data(), fmt.size());
			std::snprintf(szbuf, sizeof(szbuf), szfmt, arg);
			out += szbuf;
		}
	} else {
		out += detail::kformat_to_string(arg);
	}
}
} // namespace ktl

template <typename... Args>
void ktl::kformat_to(std::string& out, std::string_view const fmt, Args const&... args) {
	detail::kfmt{}(out, fmt, detail::format_args<detail::kfmt::size(sizeof...(Args))>::make(args...));
}

template <typename... Args>
std::string ktl::kformat(std::string_view const fmt, Args const&... args) {
	auto ret = std::string{};
	kformat_to(ret, fmt, args...);
	return ret;
}
