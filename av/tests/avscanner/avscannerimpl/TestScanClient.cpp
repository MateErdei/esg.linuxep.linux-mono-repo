/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "avscanner/avscannerimpl/ScanClient.h"
#include "tests/common/Common.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#define BASE "/tmp/TestPluginAdapter"

using namespace avscanner::avscannerimpl;

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
        /*
        virtual void cleanFile(const path&) = 0;
        virtual void infectedFile(const path&, const std::string& threatName) = 0;
         */
        MOCK_METHOD1(cleanFile, void(const path&));
        MOCK_METHOD2(infectedFile, void(const path&, const std::string&));
    };
}

using ::testing::StrictMock;

TEST(TestScanClient, TestConstruction) // NOLINT
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    ScanClient s(mock_socket, mock_callbacks, false);
}

TEST(TestScanClient, TestConstructionWithoutCallbacks) // NOLINT
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks;

    ScanClient s(mock_socket, mock_callbacks, false);
}

TEST(TestScanClient, TestScanEtcPasswd) // NOLINT
{
    using namespace ::testing;

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.setClean(true);

    EXPECT_CALL(mock_socket, scan(_,_))
        .Times(1)
        .WillOnce(Return(response));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, cleanFile(Eq("/etc/passwd")))
        .Times(1);

    ScanClient s(mock_socket, mock_callbacks, false);
    auto result = s.scan("/etc/passwd");
    EXPECT_TRUE(result.clean());
}

TEST(TestScanClient, TestScanInfectedNoCallback) // NOLINT
{
    using namespace ::testing;
    static const char* THREAT = "THREAT";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.setClean(false);
    response.setThreatName(THREAT);

    EXPECT_CALL(mock_socket, scan(_, _))
            .Times(1)
            .WillOnce(Return(response));

    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks;
    ScanClient s(mock_socket, mock_callbacks, false);
    auto result = s.scan("/etc/passwd");
    EXPECT_FALSE(result.clean());
    EXPECT_EQ(result.threatName(), THREAT);
}

TEST(TestScanClient, TestScanInfected) // NOLINT
{
    using namespace ::testing;
    static const char* THREAT = "THREAT";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.setClean(false);
    response.setThreatName(THREAT);

    EXPECT_CALL(mock_socket, scan(_, _))
            .Times(1)
            .WillOnce(Return(response));

    auto mock_callbacks = std::make_shared<StrictMock<MockIScanCallbacks> >();

    EXPECT_CALL(*mock_callbacks, infectedFile(Eq("/etc/passwd"), Eq(THREAT)))
            .Times(1);

    setupFakeSophosThreatReporterConfig();
    unixsocket::ThreatReporterServerSocket threatReporterServer(
            "/tmp/TestPluginAdapter/chroot/threat_report_socket"
            );
    threatReporterServer.start();

    ScanClient s(mock_socket, mock_callbacks, false);
    auto result = s.scan("/etc/passwd");

    threatReporterServer.requestStop();
    threatReporterServer.join();

    EXPECT_FALSE(result.clean());
    EXPECT_EQ(result.threatName(), THREAT);
    fs::remove_all("/tmp/TestPluginAdapter/");
}
