/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_threat_detector/threat_scanner/SusiLogger.h"

#include "../common/MemoryAppender.h"

#include <gtest/gtest.h>

namespace
{
    static constexpr char LOG_CATEGORY[] = "ThreatScanner";
    class TestSusiLoggerCallback : public MemoryAppenderUsingTestsTemplate<LOG_CATEGORY>
    {
    };
}

TEST_F(TestSusiLoggerCallback, debug_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_TRUE(appenderContains("DEBUG - DEBUG_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, error_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "ERROR_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - ERROR_MESSAGE"));
}


