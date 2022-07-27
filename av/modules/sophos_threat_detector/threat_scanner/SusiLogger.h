// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Susi.h>

namespace threat_scanner
{
    void susiLogCallback(void* token, SusiLogLevel level, const char* message);
    void fallbackSusiLogCallback(void* token, SusiLogLevel level, const char* message);

    class HighestLevelRecorder
    {
        static thread_local SusiLogLevel m_highest;
    public:
        static void reset()
        {
            m_highest = SUSI_LOG_LEVEL_DETAIL;
        }
        static SusiLogLevel getHighest()
        {
            return m_highest;
        }
        static void record(SusiLogLevel level);
    };
}
