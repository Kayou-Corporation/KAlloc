/// Cross-Platform compiler & performance utilities
///
#pragma once

#include <cstdint>
#include "Platform.hpp"



///
/// Compiler-specific utilities
///
#if defined(KAYOU_COMPILER_MSVC)
    /// NO_INLINE
    /// Prevents function inlining. Useful for debugging, profiling, or isolating performance costs.
    #define KAYOU_NO_INLINE __declspec(noinline)

    /// DEBUG_BREAK
    /// Triggers a breakpoint in the MSVC debugger.
    #define KAYOU_DEBUG_BREAK() __debugbreak()
#elif defined(KAYOU_COMPILER_GCC) || defined(KAYOU_COMPILER_CLANG)
    /// NO_INLINE
    /// Prevents function inlining. Useful for debugging, profiling, or isolating performance costs.
    #define KAYOU_NO_INLINE __attribute__((noinline))

    /// DEBUG_BREAK
    /// Triggers a breakpoint using a compiler intrinsic trap.
    #define KAYOU_DEBUG_BREAK() __builtin_trap()
#else
    /// NO_INLINE (fallback)
    #define KAYOU_NO_INLINE

    /// DEBUG_BREAK (fallback)
    #define KAYOU_DEBUG_BREAK()
#endif


///
/// ALWAYS_INLINE
///
/// Strong inlining hint for the compiler.
/// Compared to a normal inline hint, this is more aggressive and requests
/// that the compiler always inline the function when possible.
///
/// Useful for hot paths such as math functions, ECS inner loops, or tight simulation code.
///
#if defined(KAYOU_COMPILER_MSVC)
    #define KAYOU_ALWAYS_INLINE __forceinline
#elif defined(KAYOU_COMPILER_GCC) || defined(KAYOU_COMPILER_CLANG)
    #define KAYOU_ALWAYS_INLINE inline __attribute__((always_inline))
#else
    #define KAYOU_ALWAYS_INLINE inline
#endif


///
/// Branch prediction hints (C++20)
///
#if defined(__has_cpp_attribute)
    #if __has_cpp_attribute(likely)
        #define KAYOU_LIKELY [[likely]]
        #define KAYOU_UNLIKELY [[unlikely]]
    #else
        #define KAYOU_LIKELY
        #define KAYOU_UNLIKELY
    #endif
#else
    #define KAYOU_LIKELY
    #define KAYOU_UNLIKELY
#endif


///
/// NO_DISCARD
///
/// Prevents ignoring return values.
/// Useful for functions where the result must be checked (errors, allocations, validation).
///
#if defined(__has_cpp_attribute)
    #if __has_cpp_attribute(nodiscard)
        #define KAYOU_NO_DISCARD [[nodiscard]]
    #else
        #define KAYOU_NO_DISCARD
    #endif
#else
    #define KAYOU_NO_DISCARD
#endif


///
/// NOEXCEPT
///
/// Marks a function as not throwing exceptions.
/// Allows compiler optimizations and improves compatibility with STL containers.
///
#define KAYOU_NOEXCEPT noexcept


///
/// RESTRICT
///
/// Pointer aliasing hint that tells the compiler this pointer is unique.
/// Enables better vectorization and memory optimization.
///
#if defined(KAYOU_COMPILER_MSVC)
    #define KAYOU_RESTRICT __restrict
#elif defined(KAYOU_COMPILER_GCC) || defined(KAYOU_COMPILER_CLANG)
    #define KAYOU_RESTRICT __restrict__
#else
    #define KAYOU_RESTRICT
#endif


///
/// UNREACHABLE
///
/// Marks code as unreachable.
/// Helps compiler optimize control flow and may catch logic errors in debug builds.
///
#if defined(KAYOU_COMPILER_MSVC)
    #define KAYOU_UNREACHABLE() __assume(0)
#elif defined(KAYOU_COMPILER_GCC) || defined(KAYOU_COMPILER_CLANG)
    #define KAYOU_UNREACHABLE() __builtin_unreachable()
#else
    #define KAYOU_UNREACHABLE()
#endif


///
/// CACHELINE_ALIGN
///
/// Aligns data to a 64-byte cache line.
/// Prevents false sharing in multithreaded systems and improves cache efficiency.
/// Especially useful in ECS, physics systems, and job systems.
///
#define KAYOU_CACHELINE_ALIGN alignas(64)


///
/// Preprocessor utilities
///
#define KAYOU_STRINGIFY(x) #x
#define KAYOU_TOSTRING(x) KAYOU_STRINGIFY(x)

#define KAYOU_CONCAT(a, b) a##b
#define KAYOU_CONCAT_EXPAND(a, b) KAYOU_CONCAT(a, b)


///
/// Static assert helper
///
#define KAYOU_STATIC_ASSERT(cond, msg) static_assert(cond, msg)