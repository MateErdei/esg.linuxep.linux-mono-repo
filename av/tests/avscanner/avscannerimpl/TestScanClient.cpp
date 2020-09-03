/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "avscanner/avscannerimpl/ScanCallbackImpl.h"
#include "avscanner/avscannerimpl/ScanClient.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "tests/common/Common.h"

#define BASE "/tmp/TestPluginAdapter"

using namespace avscanner::avscannerimpl;
using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::StrictMock;

namespace fs = sophos_filesystem;

namespace
{
    class MockIScanningClientSocket : public unixsocket::IScanningClientSocket
    {
    public:
        MOCK_METHOD2(scan, scan_messages::ScanResponse(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&));
    };

    class MockIScanCallbacks : public avscanner::avscannerimpl::IScanCallbacks
    {
    public:
        MOCK_METHOD1(cleanFile, void(const path&));
        MOCK_METHOD3(infectedFile, void(const path&, const std::string&, bool isSymlink));
        MOCK_METHOD1(scanError, void(const std::string&));
    };
}

TEST(TestScanClient, TestConstruction) // NOLINT
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    ScanClient s(mock_socket, mock_callbacks, false, E_SCAN_TYPE_ON_DEMAND);
}

TEST(TestScanClient, TestConstructionWithoutCallbacks) // NOLINT
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks;

    ScanClient s(mock_socket, mock_callbacks, false, E_SCAN_TYPE_ON_DEMAND);
}

TEST(TestScanClient, TestScanEtcPasswd) // NOLINT
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection("/etc/passwd", "");

    EXPECT_CALL(mock_socket, scan(_,_))
        .Times(1)
        .WillOnce(Return(response));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, cleanFile(Eq("/etc/passwd")))
        .Times(1);

    ScanClient s(mock_socket, mock_callbacks, false, E_SCAN_TYPE_ON_DEMAND);
    auto result = s.scan("/etc/passwd");
    EXPECT_TRUE(result.allClean());
}

TEST(TestScanClient, TestScanArchive) // NOLINT
{
    path infectedFile1 = "/tmp/test.tar/eicar.com";
    path infectedFile2 = "/tmp/test.tar/eicar.exe";
    std::string threatName = "EICAR-AV-Test";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection(infectedFile1, threatName);
    response.addDetection(infectedFile2, threatName);

    EXPECT_CALL(mock_socket, scan(_,_))
        .Times(1)
        .WillOnce(Return(response));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
        new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, infectedFile(infectedFile1, threatName, false))
        .Times(1);
    EXPECT_CALL(*mock_callbacks, infectedFile(infectedFile2, threatName, false))
        .Times(1);

    ScanClient s(mock_socket, mock_callbacks, true, E_SCAN_TYPE_ON_DEMAND);
    // Note: we need to scan a file which exists on all build systems (but will pretend this is an archive for test purposes)
    auto result = s.scan("/etc/passwd");
    EXPECT_FALSE(result.allClean());
    EXPECT_EQ(result.getDetections().size(), 2);
}

TEST(TestScanClient, TestScanInfectedNoCallback) // NOLINT
{
    using namespace ::testing;
    static const char* THREAT = "THREAT";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection("/tmp/eicar.com", THREAT);

    EXPECT_CALL(mock_socket, scan(_, _))
            .Times(1)
            .WillOnce(Return(response));

    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks;
    ScanClient s(mock_socket, mock_callbacks, false, E_SCAN_TYPE_ON_DEMAND);
    auto result = s.scan("/etc/passwd");
    EXPECT_FALSE(result.getDetections().empty());
    EXPECT_EQ(result.getDetections()[0].second, THREAT);
}

TEST(TestScanClient, TestScanInfected) // NOLINT
{
    using namespace ::testing;
    static const char* THREAT = "THREAT";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection("/etc/passwd", THREAT);

    EXPECT_CALL(mock_socket, scan(_, _))
        .Times(1)
        .WillOnce(Return(response));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, infectedFile(Eq("/etc/passwd"), Eq(THREAT), false))
            .Times(1);

    ScanClient s(mock_socket, mock_callbacks, false, E_SCAN_TYPE_ON_DEMAND);
    auto result = s.scan("/etc/passwd");

    EXPECT_FALSE(result.getDetections().empty());
    EXPECT_EQ(result.getDetections()[0].second, THREAT);
}
