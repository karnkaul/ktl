#pragma once
#include <shared_mutex>
#include "tmutex.hpp"

namespace ktl {
///
/// \brief Alias for std::shared_mutex
///
template <typename T, typename M = std::shared_mutex>
using shared_tmutex = tmutex<T, M>;
///
/// \brief Alias for std::shared_mutex (strict)
///
template <typename T, typename M = std::shared_mutex>
using shared_strict_tmutex = tmutex<T, M>;
///
/// \brief Alias for std::shared_lock
///
template <typename T, template <typename...> typename L = std::shared_lock, typename M = std::shared_mutex>
using shared_tlock = tlock<T, L, M>;
///
/// \brief Alias for std::unique_lock
///
template <typename T, template <typename...> typename L = std::unique_lock, typename M = std::shared_mutex>
using unique_tlock = tlock<T, L, M>;
} // namespace ktl
