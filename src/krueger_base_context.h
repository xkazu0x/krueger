#ifndef KRUEGER_BASE_CONTEXT_H
#define KRUEGER_BASE_CONTEXT_H

///////////////////////////////
// NOTE: Clang Context Cracking

#if defined(__clang__)
#define COMPILER_CLANG 1

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MAC 1
#error compiler/platform not supported.
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define ARCH_X86 1
#elif defined(__aarch64__)
#define ARCH_ARM64 1
#elif defined(__arm__)
#define ARCH_ARM32 1
#else
#error architecture not supported.
#endif

//////////////////////////////
// NOTE: MSVC Context Cracking

#elif defined(_MSC_VER)
#define COMPILER_MSVC 1

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#else
#error compiler/platform not supported.
#endif

#if defined(_M_AMD64)
#define ARCH_X64 1
#elif defined(_M_IX86)
#define ARCH_X86 1
#elif defined(_M_ARM64)
#define ARCH_ARM64 1
#elif defined(_M_ARM)
#define ARCH_ARM32 1
#else
#error architecture not supported.
#endif

/////////////////////////////
// NOTE: GCC Context Cracking

#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_GCC 1

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MAC 1
#else
#error compiler/platform not supported.
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define ARCH_X86 1
#elif defined(__aarch64__)
#define ARCH_ARM64 1
#elif defined(__arm__)
#define ARCH_ARM32 1
#else
#error architecture not supported.
#endif

#else
#error compiler not supported.
#endif

//////////////////////////////
// NOTE: Build Option Cracking

#if !defined(BUILD_DEBUG)
#define BUILD_DEBUG 1
#endif

///////////////////////////////////
// NOTE: Zero All Undefined Options

#if !defined(COMPILER_CLANG)
#define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
#define COMPILER_GCC 0
#endif
#if !defined(PLATFORM_WINDOWS)
#define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_LINUX)
#define PLATFORM_LINUX 0
#endif
#if !defined(PLATFORM_MAC)
#define PLATFORM_MAC 0
#endif
#if !defined(ARCH_X64)
#define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
#define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
#define ARCH_ARM32 0
#endif

#endif // KRUEGER_BASE_CONTEXT_H
