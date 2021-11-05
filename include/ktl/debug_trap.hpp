#pragma once

#if defined(KTL_DEBUG_TRAP)
#undef KTL_DEBUG_TRAP
#endif

// clang-format off
#if defined(KTL_NO_DEBUG_TRAP)
	#define KTL_DEBUG_TRAP()
#elif defined(_MSC_VER)
	#define KTL_DEBUG_TRAP_ENABLED
	#define KTL_DEBUG_TRAP() __debugbreak()
#elif defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
	#define KTL_DEBUG_TRAP_ENABLED
	#define KTL_DEBUG_TRAP() asm volatile("int3")
#elif defined(__clang__)
	#define KTL_DEBUG_TRAP_ENABLED
	#define KTL_DEBUG_TRAP() __builtin_debugtrap()
#else
	#include <csignal>
	#if defined(SIGTRAP)
		#define KTL_DEBUG_TRAP_ENABLED
		#define KTL_DEBUG_TRAP() raise(SIGTRAP)
	#else
		#define KTL_DEBUG_TRAP()
	#endif
#endif
// clang-format on
