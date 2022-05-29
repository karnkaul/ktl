// KTL single-header library
// Requirements: C++20

#pragma once
#include <compare>

namespace ktl {
///
/// \brief Semantic version
///
struct kversion {
	int major{};
	int minor{};
	int patch{};

	auto operator<=>(kversion const&) const = default;

	template <typename OstreamT>
	friend constexpr OstreamT& operator<<(OstreamT& out, kversion const& v) {
		return out << 'v' << v.major << '.' << v.minor << '.' << v.patch;
	}

	template <typename IstreamT>
	friend constexpr IstreamT& operator<<(IstreamT& in, kversion& out) {
		char discard;
		return in >> discard >> out.major >> discard >> out.minor >> discard >> out.patch;
	}
};
} // namespace ktl
