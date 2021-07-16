/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <cstdint>

// Windows types used by Journal library
using DWORD = uint32_t;
struct FILETIME
{
    uint32_t lowDateTime;
    uint32_t highDateTime;
};

namespace Common
{
    namespace EventJournalWrapper
    {
        // Windows FILETIME contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC)
        // https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
        static inline uint64_t getWindowsFileTime(const FILETIME& ft)
        {
            return (static_cast<uint64_t>(ft.highDateTime) << 32) + ft.lowDateTime;
        }

        // copied from SED
        static constexpr uint64_t WINDOWS_FILETIME_OFFSET = 0x019db1ded53e8000;
        static constexpr uint64_t WINDOWS_100NANO_PER_SECOND = 10000000;

        static inline time_t WindowsFileTimeToUNIX(uint64_t ft)
        {
            return (ft - WINDOWS_FILETIME_OFFSET) / WINDOWS_100NANO_PER_SECOND;
        }

        static inline uint64_t UNIXToWindowsFileTime(time_t t)
        {
            return (static_cast<uint64_t>(t) * WINDOWS_100NANO_PER_SECOND) + WINDOWS_FILETIME_OFFSET;
        }
    } // namespace EventJournalWrapper
} // namespace Common
