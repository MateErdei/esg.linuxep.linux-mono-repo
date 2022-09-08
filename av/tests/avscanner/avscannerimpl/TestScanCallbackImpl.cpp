// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>

#include "avscanner/avscannerimpl/ScanCallbackImpl.h"
#include "tests/common/LogInitializedTests.h"
#include <gtest/gtest.h>

using namespace avscanner::avscannerimpl;
using namespace common;

class TestScanCallbackImpl : public LogInitializedTests
{
};

TEST_F(TestScanCallbackImpl, TestErrorReturnCode)
{
    ScanCallbackImpl scanCallback;
    std::error_code ec (EINVAL, std::system_category());
    scanCallback.scanError("An error occurred.", ec);
    EXPECT_EQ(scanCallback.returnCode(), E_GENERIC_FAILURE);
}

TEST_F(TestScanCallbackImpl, TestInfectedReturnCode)
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;

    scanCallback.infectedFile(detections, "", "", false);
    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}

TEST_F(TestScanCallbackImpl, TestCleanReturnCode)
{
    ScanCallbackImpl scanCallback;
    scanCallback.cleanFile("");
    EXPECT_EQ(scanCallback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestScanCallbackImpl, TestInfectedOverCleanReturnCode)
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;

    for (int i=0; i<5; i++)
    {
        scanCallback.cleanFile("");
        scanCallback.infectedFile(detections, "", "", false);
    }

    scanCallback.cleanFile("");

    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}

TEST_F(TestScanCallbackImpl, TestErrorOverCleanReturnCode)
{
    ScanCallbackImpl scanCallback;

    for (int i=0; i<5; i++)
    {
        scanCallback.cleanFile("");
        std::error_code ec (EINVAL, std::system_category());
        scanCallback.scanError("An error occurred.", ec);
    }

    scanCallback.cleanFile("");

    EXPECT_EQ(scanCallback.returnCode(), E_GENERIC_FAILURE);
}

TEST_F(TestScanCallbackImpl, TestInfectedOverErrorReturnCode)
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;
    std::error_code ec (EINVAL, std::system_category());

    for (int i=0; i<5; i++)
    {
        scanCallback.infectedFile(detections, "", "", false);
        scanCallback.scanError("An error occurred.", ec);
    }

    scanCallback.scanError("An error occurred.", ec);

    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}
