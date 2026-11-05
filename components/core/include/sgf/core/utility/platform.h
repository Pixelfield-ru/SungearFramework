//
// Created by stuka on 05.11.2026.
//

#pragma once

#define SGF_PLATFORM_COMPILER_CLANG 0
#define SGF_PLATFORM_COMPILER_GCC 0
#define SGF_PLATFORM_COMPILER_MSVC 0

#define SGF_PLATFORM_OS_ANDROID 0
#define SGF_PLATFORM_OS_LINUX 0
#define SGF_PLATFORM_OS_WINDOWS 0
#define SGF_PLATFORM_OS_IOS 0
#define SGF_PLATFORM_OS_MAC_OS_X 0

#define SGF_PLATFORM_PC 0
#define SGF_PLATFORM_MOBILE 0

#if defined(__clang__)
#undef SGF_PLATFORM_COMPILER_CLANG
#define SGF_PLATFORM_COMPILER_CLANG 1
#elif defined(__GNUC__)
#undef SGF_PLATFORM_COMPILER_GCC
#define SGF_PLATFORM_COMPILER_GCC 1
#elif defined(_MSC_VER)
#undef SGF_PLATFORM_COMPILER_MSVC
#define SGF_PLATFORM_COMPILER_MSVC 1
#else
#error "Unsupported compiler. Supported compilers are: MSVC, GCC, Clang."
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__linux__) && defined(__ANDROID__)
#undef SGF_PLATFORM_OS_ANDROID
#define SGF_PLATFORM_OS_ANDROID 1
#elif defined(__linux__)
#undef SGF_PLATFORM_OS_LINUX
#define SGF_PLATFORM_OS_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
#undef SGF_PLATFORM_OS_WINDOWS
#define SGF_PLATFORM_OS_WINDOWS 1
#elif defined(__APPLE__) && TARGET_OS_IPHONE == 1
#undef SGF_PLATFORM_OS_IOS
#define SGF_PLATFORM_OS_IOS 1
#elif defined(__APPLE__) && TARGET_OS_MAC == 1
#undef SGF_PLATFORM_OS_MAC_OS_X
#define SGF_PLATFORM_OS_MAC_OS_X 1
#else
#error "Unsupported OS. Supported operating systems are: Windows, Linux, Mac OS X, Android, iOS"
#endif

#if SGF_PLATFORM_OS_WINDOWS || SGF_PLATFORM_OS_LINUX || SGF_PLATFORM_OS_MAC_OS_X
#undef SGF_PLATFORM_PC
#define SGF_PLATFORM_PC 1
#elif SGF_PLATFORM_OS_ANDROID || SGF_PLATFORM_OS_IOS
#undef SGF_PLATFORM_MOBILE
#define SGF_PLATFORM_MOBILE 1
#endif