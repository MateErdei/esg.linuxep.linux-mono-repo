/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>

#include "avscanner/avscannerimpl/ScanCallbackImpl.h"
#include "tests/common/LogInitializedTests.h"
#include <gtest/gtest.h>

using namespace avscanner::avscannerimpl;
using namespace common;

class TestScanCallbackImpl : public LogInitializedTests
{
};

TEST_F(TestScanCallbackImpl, TestErrorReturnCode) // NOLINT
{
    ScanCallbackImpl scanCallback;
    scanCallback.scanError("An error occurred.");
    EXPECT_EQ(scanCallback.returnCode(), E_GENERIC_FAILURE);
}

TEST_F(TestScanCallbackImpl, TestInfectedReturnCode) // NOLINT
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;

    scanCallback.infectedFile(detections, "", false);
    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}

TEST_F(TestScanCallbackImpl, TestCleanReturnCode) // NOLINT
{
    ScanCallbackImpl scanCallback;
    scanCallback.cleanFile("");
    EXPECT_EQ(scanCallback.returnCode(), E_CLEAN);
}

TEST_F(TestScanCallbackImpl, TestInfectedOverCleanReturnCode) // NOLINT
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;

    for (int i=0; i<5; i++)
    {
        scanCallback.cleanFile("");
        scanCallback.infectedFile(detections, "", false);
    }

    scanCallback.cleanFile("");

    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}

TEST_F(TestScanCallbackImpl, TestErrorOverCleanReturnCode) // NOLINT
{
    ScanCallbackImpl scanCallback;

    for (int i=0; i<5; i++)
    {
        scanCallback.cleanFile("");
        scanCallback.scanError("An error occurred.");
    }

    scanCallback.cleanFile("");

    EXPECT_EQ(scanCallback.returnCode(), E_GENERIC_FAILURE);
}

TEST_F(TestScanCallbackImpl, TestInfectedOverErrorReturnCode) // NOLINT
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;

    for (int i=0; i<5; i++)
    {
        scanCallback.infectedFile(detections, "", false);
        scanCallback.scanError("An error occurred.");
    }

    scanCallback.scanError("An error occurred.");

    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}

TEST_F(TestScanCallbackImpl, TestLessThanSecond) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 0;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestScanCallbackImpl, TestScanDurationMoreThanASecond) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 4;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 4);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestScanCallbackImpl, TestScanDurationUnderAMinute) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 59;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 59);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestScanCallbackImpl, TestScanDurationAMinute) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 60;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 1);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestScanCallbackImpl, TestScanDurationMoreThanAMinute) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 121;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 2);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestScanCallbackImpl, TestScanDurationUnderAnHour) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 3599;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 59);
    EXPECT_EQ(result.min, 59);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestScanCallbackImpl, TestScanDurationAnHour) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 3600;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 1);
}

TEST_F(TestScanCallbackImpl, TestScanDurationMoreThanAnHour) // NOLINT
{
    ScanCallbackImpl scanCallback;
    auto totalScanTime = 12615;

    ScanCallbackImpl::TimeDuration result = scanCallback.timeConversion(totalScanTime);

    EXPECT_EQ(result.sec, 15);
    EXPECT_EQ(result.min, 30);
    EXPECT_EQ(result.hour, 3);
}