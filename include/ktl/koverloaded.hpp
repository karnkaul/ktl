#pragma once

namespace ktl {
///
/// \brief Wrapper for visitor
///
template <typename... T>
struct koverloaded : T... {
	using T::operator()...;
};
template <typename... T>
koverloaded(T...) -> koverloaded<T...>;
} // namespace ktl
