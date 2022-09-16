// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanRunnerMemoryAppenderUsingTests.h"

#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>

#include "avscanner/avscannerimpl/ScanCallbackImpl.h"
#include "tests/common/LogInitializedTests.h"
#include <gtest/gtest.h>

using namespace avscanner::avscannerimpl;
using namespace common;

class TestScanCallbackImpl : public ScanRunnerMemoryAppenderUsingTests
{
};

TEST_F(TestScanCallbackImpl, TestErrorReturnCode)
{
    ScanCallbackImpl scanCallback;
    std::error_code ec (EINVAL, std::system_category());
    scanCallback.scanError("An error occurred.", ec);
    EXPECT_EQ(scanCallback.returnCode(), E_GENERIC_FAILURE);
}

TEST_F(TestScanCallbackImpl, TestInfectedReturnCodeNoDetections)
{
    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;

    scanCallback.infectedFile(detections, "", "", false);
    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);
}

TEST_F(TestScanCallbackImpl, TestInfectedReturnCodeOneDetection)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;
    detections["/expected"] = "ThreatName";

    scanCallback.infectedFile(detections, "/path", "Scheduled", false);
    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);

    EXPECT_TRUE(appenderContains("Detected \"/expected\" is infected with ThreatName (Scheduled)"));
}

TEST_F(TestScanCallbackImpl, TestInfectedReturnCodeOneDetectionSymlink)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanCallbackImpl scanCallback;
    std::map<path, std::string> detections;
    detections["/expected"] = "ThreatName";

    scanCallback.infectedFile(detections, "/tmp", "On Demand", true);
    EXPECT_EQ(scanCallback.returnCode(), E_VIRUS_FOUND);

    EXPECT_TRUE(appenderContains(
        "Detected \"/expected\" (symlinked to /tmp) is infected with ThreatName (On Demand)"));
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

TEST_F(TestScanCallbackImpl, logSummary)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanCallbackImpl scanCallback;
    scanCallback.scanStarted();

    std::map<path, std::string> detections;
    detections["/expected"] = "ThreatName";
    scanCallback.infectedFile(detections, "/tmp", "On Demand", true);

    scanCallback.cleanFile("");

    detections.clear();
    scanCallback.infectedFile(detections, "/d2", "Scheduled", false);

    scanCallback.logSummary();

    EXPECT_TRUE(appenderContains("End of Scan Summary:"));
    EXPECT_TRUE(appenderContains("3 files scanned in "));
    EXPECT_TRUE(appenderContains("2 files out of 3 were infected."));
    EXPECT_TRUE(appenderContains("1 ThreatName infection discovered."));
}
