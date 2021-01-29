/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_threat_detector/threat_scanner/SusiLogger.h"

#include "../common/MemoryAppender.h"

#include <gtest/gtest.h>

namespace
{
    constexpr char LOG_CATEGORY[] = "ThreatScanner";
    class TestSusiLoggerCallback : public MemoryAppenderUsingTestsTemplate<LOG_CATEGORY>
    {
    };
    constexpr char SUSI_LOG_CATEGORY[] = "SUSI_DEBUG";
    class TestSusiLoggerCallbackSusiDebugLog : public MemoryAppenderUsingTestsTemplate<SUSI_LOG_CATEGORY>
    {
    };
}

// Test SUSI debug stream:

TEST_F(TestSusiLoggerCallbackSusiDebugLog, debug_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_TRUE(appenderContains("DEBUG - DEBUG_MESSAGE"));
}

TEST_F(TestSusiLoggerCallbackSusiDebugLog, info_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_INFO, "INFO_MESSAGE");
    EXPECT_TRUE(appenderContains("INFO - INFO_MESSAGE"));
}

TEST_F(TestSusiLoggerCallbackSusiDebugLog, warning_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_WARNING, "WARNING_MESSAGE");
    EXPECT_TRUE(appenderContains("WARN - WARNING_MESSAGE"));
}

TEST_F(TestSusiLoggerCallbackSusiDebugLog, error_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "ERROR_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - ERROR_MESSAGE"));
}

// Test normal log stream:

TEST_F(TestSusiLoggerCallback, debug_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_FALSE(appenderContains("DEBUG_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, warning_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_WARNING, "WARNING_MESSAGE");
    EXPECT_TRUE(appenderContains("WARN - WARNING_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, error_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "ERROR_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - ERROR_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, unknown_level) // NOLINT
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, static_cast<SusiLogLevel>(50), "UNKNOWN_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - 50: UNKNOWN_MESSAGE"));
}
