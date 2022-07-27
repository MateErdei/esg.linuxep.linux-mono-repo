// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "../../common/MemoryAppender.h"
#include "sophos_threat_detector/threat_scanner/SusiLogger.h"

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

TEST_F(TestSusiLoggerCallbackSusiDebugLog, debug_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_TRUE(appenderContains("DEBUG - DEBUG_MESSAGE"));
}

TEST_F(TestSusiLoggerCallbackSusiDebugLog, info_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_INFO, "INFO_MESSAGE");
    EXPECT_TRUE(appenderContains("SPRT - INFO_MESSAGE"));
}

TEST_F(TestSusiLoggerCallbackSusiDebugLog, warning_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_WARNING, "WARNING_MESSAGE");
    EXPECT_TRUE(appenderContains("WARN - WARNING_MESSAGE"));
}

TEST_F(TestSusiLoggerCallbackSusiDebugLog, error_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "ERROR_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - ERROR_MESSAGE"));
}

// Test normal log stream:

TEST_F(TestSusiLoggerCallback, debug_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_FALSE(appenderContains("DEBUG_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, warning_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_WARNING, "WARNING_MESSAGE");
    EXPECT_TRUE(appenderContains("WARN - WARNING_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, error_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "ERROR_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - ERROR_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, unknown_level)
{
    UsingMemoryAppender holder(*this);
    threat_scanner::susiLogCallback(nullptr, static_cast<SusiLogLevel>(50), "UNKNOWN_MESSAGE");
    EXPECT_TRUE(appenderContains("ERROR - 50: UNKNOWN_MESSAGE"));
}

TEST_F(TestSusiLoggerCallback, highest_is_debug)
{
    using namespace threat_scanner;
    HighestLevelRecorder::reset();
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_EQ(HighestLevelRecorder::getHighest(), SUSI_LOG_LEVEL_DETAIL);
}

TEST_F(TestSusiLoggerCallback, highest_is_info)
{
    // Also tests the highest first.

    using namespace threat_scanner;
    HighestLevelRecorder::reset();
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_INFO, "MESSAGE");
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "DEBUG_MESSAGE");
    EXPECT_EQ(HighestLevelRecorder::getHighest(), SUSI_LOG_LEVEL_INFO);
}

TEST_F(TestSusiLoggerCallback, highest_is_warning)
{
    // Also tests the highest last.

    using namespace threat_scanner;
    HighestLevelRecorder::reset();
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "MESSAGE");
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_WARNING, "MESSAGE");
    EXPECT_EQ(HighestLevelRecorder::getHighest(), SUSI_LOG_LEVEL_WARNING);
}

TEST_F(TestSusiLoggerCallback, highest_is_error)
{
    // Also tests the highest in the middle.

    using namespace threat_scanner;
    HighestLevelRecorder::reset();
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_DETAIL, "MESSAGE");
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "MESSAGE");
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_WARNING, "MESSAGE");
    EXPECT_EQ(HighestLevelRecorder::getHighest(), SUSI_LOG_LEVEL_ERROR);
}

TEST_F(TestSusiLoggerCallback, highest_is_debug_after_reset)
{
    using namespace threat_scanner;
    HighestLevelRecorder::reset();
    susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "MESSAGE");
    EXPECT_EQ(HighestLevelRecorder::getHighest(), SUSI_LOG_LEVEL_ERROR);

    HighestLevelRecorder::reset();
    EXPECT_EQ(HighestLevelRecorder::getHighest(), SUSI_LOG_LEVEL_DETAIL);
}
