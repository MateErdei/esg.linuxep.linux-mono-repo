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

        static inline FILETIME getFileTimeFromWindowsTime(const int64_t windowsTime)
        {
            FILETIME fileTime = { };
            fileTime.highDateTime = static_cast<uint32_t>((windowsTime >> 32) & 0xffffffff);
            fileTime.lowDateTime = static_cast<uint32_t>(windowsTime & 0xffffffff);
            return fileTime;
        }
    } // namespace EventJournalWrapper
} // namespace Common
