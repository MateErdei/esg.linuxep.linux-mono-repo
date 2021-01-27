// Copyright 2020 Sophos Limited.

#pragma once

#include <Windows.h>

namespace CppCommon::WindowsOnWindows
{
#if defined _M_IX86
    // A process that targets Win32/x86 could be running on a Win32 or x64 platform.
    // SAU does not support Win32/x86 binaries on ARM64; that would require run-time determination of redirection flags.
    // Therefore, ARM64 components must provide a setup64.dll.
    // On Win32, redirection is irrelevant since there is only one hive.
    // On x64, for legacy reasons, SAU uses 32-bit registry keys and therefore requires redirection.
    constexpr auto keyWow = KEY_WOW64_32KEY;
#elif defined _M_X64
    // A process that targets x64 must be running on an x64 platform.
    // On x64, for legacy reasons, SAU uses 32-bit registry keys and therefore requires redirection.
    constexpr auto keyWow = KEY_WOW64_32KEY;
#elif defined _M_ARM64
    // A process that targets ARM64 must be running on an ARM64 platform.
    // On ARM64, SAU uses native registry keys and therefore has no redirection.
    // SAU is not supported on 32-bit ARM.
    constexpr auto keyWow = 0;
#else
    static_assert(false, "Unrecognised platform");
#endif

    constexpr auto useWow = 0 != keyWow;
} // namespace CppCommon::WindowsOnWindows
