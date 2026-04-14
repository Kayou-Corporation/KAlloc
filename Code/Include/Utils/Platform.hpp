/// Cross-Platform / Compiler / Architecture detection
#pragma once

#include <cstdint>      // GCC std::uint32_t & std::uintptr_t



/// Compiler detection
#if defined(__clang__)
    #define KAYOU_COMPILER_CLANG 1
    #define KAYOU_COMPILER_NAME "Clang"
    #define KAYOU_COMPILER_VERSION (__clang_major__ * 100 + __clang_minor__)
#elif defined(__GNUC__)
    #define KAYOU_COMPILER_GCC 1
    #define KAYOU_COMPILER_NAME "GCC"
    #define KAYOU_COMPILER_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#elif defined(_MSC_VER)
    #define KAYOU_COMPILER_MSVC 1
    #define KAYOU_COMPILER_NAME "MSVC"
    #define KAYOU_COMPILER_VERSION _MSC_VER
#else
    #define KAYOU_COMPILER_UNKNOWN 1
    #define KAYOU_COMPILER_NAME "Unknown"
#endif


/// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define KAYOU_PLATFORM_WINDOWS 1
    #define KAYOU_PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define KAYOU_PLATFORM_LINUX 1
    #define KAYOU_PLATFORM_NAME "Linux"
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define KAYOU_PLATFORM_IOS 1
        #define KAYOU_PLATFORM_NAME "iOS"
    #else
        #define KAYOU_PLATFORM_MACOS 1
        #define KAYOU_PLATFORM_NAME "macOS"
    #endif
#else
    #define KAYOU_PLATFORM_UNKNOWN 1
    #define KAYOU_PLATFORM_NAME "Unknown"
#endif


/// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define KAYOU_ARCH_X64 1
    #define KAYOU_ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define KAYOU_ARCH_X86 1
    #define KAYOU_ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define KAYOU_ARCH_ARM64 1
    #define KAYOU_ARCH_NAME "ARM64"
#elif defined(__arm__) || defined(_M_ARM)
    #define KAYOU_ARCH_ARM 1
    #define KAYOU_ARCH_NAME "ARM"
#else
    #define KAYOU_ARCH_UNKNOWN 1
    #define KAYOU_ARCH_NAME "Unknown"
#endif


/// Build configuration
#if defined(NDEBUG)
    #define KAYOU_BUILD_RELEASE 1
#else
    #define KAYOU_BUILD_DEBUG 1
#endif


/// Compile-time helper constants
namespace Kayou::Utils::Platform
{
    /// Compiler name (string literal)
    inline constexpr const char* Compiler = KAYOU_COMPILER_NAME;

    /// Operating system name (string literal)
    inline constexpr const char* OS = KAYOU_PLATFORM_NAME;

    /// Architecture name (string literal)
    inline constexpr const char* Arch = KAYOU_ARCH_NAME;


    /// True if debug build
    inline constexpr bool IsDebug =
    #if defined(KAYOU_BUILD_DEBUG)
        true;
    #else
        false;
    #endif


    /// True if release build
    inline constexpr bool IsRelease =
    #if defined(KAYOU_BUILD_RELEASE)
        true;
    #else
        false;
    #endif


    /// True if Windows platform
    inline constexpr bool IsWindows =
    #if defined(KAYOU_PLATFORM_WINDOWS)
        true;
    #else
        false;
    #endif


    /// True if Linux platform
    inline constexpr bool IsLinux =
    #if defined(KAYOU_PLATFORM_LINUX)
        true;
    #else
        false;
    #endif


    /// True if macOS platform
    inline constexpr bool IsMacOS =
    #if defined(KAYOU_PLATFORM_MACOS)
        true;
    #else
        false;
    #endif


    /// True if x64 architecture
    inline constexpr bool IsX64 =
    #if defined(KAYOU_ARCH_X64)
        true;
    #else
        false;
    #endif


    /// True if ARM64 architecture
    inline constexpr bool IsARM64 =
    #if defined(KAYOU_ARCH_ARM64)
        true;
    #else
        false;
    #endif

} // namespace Kayou::Utils::Platform