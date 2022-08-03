/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "ScanRunnerMemoryAppenderUsingTests.h"

#include "RecordingMockSocket.h"

#include "avscanner/avscannerimpl/ClientSocketWrapper.h"


using namespace ::testing;
using namespace avscanner::avscannerimpl;

namespace
{
    class TestClientSocketWrapper : public ScanRunnerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override {}

        void TearDown() override {}
    };
}

TEST_F(TestClientSocketWrapper, Construction)
{
    RecordingMockSocket socket {};
    ClientSocketWrapper csw {socket};
}

TEST_F(TestClientSocketWrapper, Scan)
{
    datatypes::AutoFd fd {};
    scan_messages::ClientScanRequest request;
    request.setPath("/foo/bar");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    RecordingMockSocket socket {};
    ClientSocketWrapper csw {socket};

    auto response = csw.scan(fd, request);
    EXPECT_FALSE(response.allClean());

    auto detections = response.getDetections();
    ASSERT_EQ(detections.size(), 1);

    auto detection = detections.at(0);
    EXPECT_EQ(detection.path, "/foo/bar");
}
