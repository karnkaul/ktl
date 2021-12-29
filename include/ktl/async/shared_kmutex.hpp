#pragma once
#include <shared_mutex>
#include "kmutex.hpp"

namespace ktl {
///
/// \brief Alias for std::shared_mutex
///
template <typename T, typename M = std::shared_mutex>
using shared_kmutex = kmutex<T, M>;
///
/// \brief Alias for std::shared_mutex (strict)
///
template <typename T, typename M = std::shared_mutex>
using shared_strict_tmutex = kmutex<T, M>;
///
/// \brief Alias for std::shared_lock
///
template <typename T, template <typename...> typename L = std::shared_lock, typename M = std::shared_mutex>
using shared_klock = klock<T, L, M>;
///
/// \brief Alias for std::unique_lock
///
template <typename T, template <typename...> typename L = std::unique_lock, typename M = std::shared_mutex>
using unique_klock = klock<T, L, M>;
} // namespace ktl
