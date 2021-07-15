/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <cstdint>

namespace EventJournal
{
    // Windows FILETIME contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC)
    // https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime

    // copied from SED
    static constexpr uint64_t WINDOWS_FILETIME_OFFSET = 0x019db1ded53e8000;
    static constexpr uint64_t WINDOWS_100NANO_PER_SECOND = 10000000;

    static inline time_t WindowsFileTimeToUNIX(int64_t ft)
    {
        return (ft - WINDOWS_FILETIME_OFFSET) / WINDOWS_100NANO_PER_SECOND;
    }

    static inline int64_t UNIXToWindowsFileTime(time_t t)
    {
        return (static_cast<int64_t>(t) * WINDOWS_100NANO_PER_SECOND) + WINDOWS_FILETIME_OFFSET;
    }
} // namespace EventJournal
