// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "avscanner/avscannerimpl/ScanClient.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>

using namespace avscanner::avscannerimpl;
using ::testing::_; // NOLINT
using ::testing::Eq;
using ::testing::Return;
using ::testing::StrictMock;

namespace fs = sophos_filesystem;

namespace
{
    class MockIScanningClientSocket : public unixsocket::IScanningClientSocket
    {
    public:
        MockIScanningClientSocket()
        {
            m_socketFd.reset(::open("/dev/zero", O_RDONLY));
            ON_CALL(*this, socketFd).WillByDefault([this]() {return this->m_socketFd.fd();});
        }

        MOCK_METHOD(int, connect, ());
        MOCK_METHOD(bool, sendRequest, (datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request));
        MOCK_METHOD(bool, receiveResponse, (scan_messages::ScanResponse& response));
        MOCK_METHOD(int, socketFd, ());

    private:
        datatypes::AutoFd m_socketFd;
    };

    class MockIScanCallbacks : public avscanner::avscannerimpl::IScanCallbacks
    {
    public:
        MOCK_METHOD(void, cleanFile, (const path&));
        MOCK_METHOD(void, infectedFile, ((const std::map<path, std::string>& detections), const fs::path& realPath,  bool isSymlink));
        MOCK_METHOD(void, scanError, (const std::string&, std::error_code), (override));
        MOCK_METHOD(void, scanStarted, ());
        MOCK_METHOD(void, logSummary, ());
    };
}

TEST(TestScanClient, TestConstruction)
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));

    ScanClient s(mock_socket, mock_callbacks, false, false, E_SCAN_TYPE_ON_DEMAND);
}

TEST(TestScanClient, TestConstructionWithoutCallbacks)
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks;

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));

    ScanClient s(mock_socket, mock_callbacks, false, false, E_SCAN_TYPE_ON_DEMAND);
}

TEST(TestScanClient, TestScanEtcPasswd)
{
    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_socket, socketFd)
        .Times(1);
    EXPECT_CALL(mock_socket, sendRequest(_,_))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mock_socket, receiveResponse(_))
        .Times(1)
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(response),
            Return(true)
                ));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, cleanFile(Eq("/etc/passwd")))
        .Times(1);

    ScanClient s(mock_socket, mock_callbacks, false, false, E_SCAN_TYPE_ON_DEMAND);
    auto result = s.scan("/etc/passwd");
    EXPECT_TRUE(result.allClean());
}

TEST(TestScanClient, TestScanArchive)
{
    path infectedFile1 = "/tmp/test.tar/eicar.com";
    path infectedFile2 = "/tmp/test.tar/eicar.exe";
    std::string threatName = "EICAR-AV-Test";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection(infectedFile1, threatName,"");
    response.addDetection(infectedFile2, threatName,"");
    std::map<path, std::string> detections;
    detections.emplace(infectedFile1, threatName);
    detections.emplace(infectedFile2, threatName);

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_socket, socketFd);
    EXPECT_CALL(mock_socket, sendRequest(_,_))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mock_socket, receiveResponse(_))
        .Times(1)
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(response),
            Return(true)
        ));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
        new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, infectedFile(detections, _, false))
        .Times(1);

    ScanClient s(mock_socket, mock_callbacks, true, false, E_SCAN_TYPE_ON_DEMAND);
    // Note: we need to scan a file which exists on all build systems (but will pretend this is an archive for test purposes)
    auto result = s.scan("/etc/passwd");
    EXPECT_FALSE(result.allClean());
    EXPECT_EQ(result.getDetections().size(), 2);
}

TEST(TestScanClient, TestScanImage)
{
    path infectedFile1 = "/tmp/test.tar/eicar.iso";
    std::string threatName = "EICAR-AV-Test";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection(infectedFile1, threatName,"");
    std::map<path, std::string> detections;
    detections.emplace(infectedFile1, threatName);

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_socket, socketFd);
    EXPECT_CALL(mock_socket, sendRequest(_,_))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mock_socket, receiveResponse(_))
        .Times(1)
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(response),
            Return(true)
                ));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, infectedFile(detections, _, false))
            .Times(1);

    ScanClient s(mock_socket, mock_callbacks, false, true, E_SCAN_TYPE_ON_DEMAND);
    // Note: we need to scan a file which exists on all build systems (but will pretend this is an image for test purposes)
    auto result = s.scan("/etc/passwd");
    EXPECT_FALSE(result.allClean());
    EXPECT_EQ(result.getDetections().size(), 1);
}

TEST(TestScanClient, TestScanInfectedNoCallback)
{
    using namespace ::testing;
    static const char* THREAT = "THREAT";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection("/tmp/eicar.com", THREAT,"");

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_socket, socketFd);
    EXPECT_CALL(mock_socket, sendRequest(_,_))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mock_socket, receiveResponse(_))
        .Times(1)
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(response),
            Return(true)
                ));

    std::shared_ptr<avscanner::avscannerimpl::IScanCallbacks> mock_callbacks;
    ScanClient s(mock_socket, mock_callbacks, false, false, E_SCAN_TYPE_ON_DEMAND);
    auto result = s.scan("/etc/passwd");
    EXPECT_FALSE(result.getDetections().empty());
    EXPECT_EQ(result.getDetections()[0].name, THREAT);
}

TEST(TestScanClient, TestScanInfected)
{
    using namespace ::testing;
    static const char* THREAT = "THREAT";

    StrictMock<MockIScanningClientSocket> mock_socket;
    scan_messages::ScanResponse response;
    response.addDetection("/etc/passwd", THREAT,"");
    std::map<path, std::string> detections;
    detections.emplace("/etc/passwd", THREAT);

    EXPECT_CALL(mock_socket, connect)
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(mock_socket, socketFd);
    EXPECT_CALL(mock_socket, sendRequest(_,_))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(mock_socket, receiveResponse(_))
        .Times(1)
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(response),
            Return(true)
                ));

    std::shared_ptr<StrictMock<MockIScanCallbacks> > mock_callbacks(
            new StrictMock<MockIScanCallbacks>()
    );

    EXPECT_CALL(*mock_callbacks, infectedFile(Eq(detections), _, false))
            .Times(1);

    ScanClient s(mock_socket, mock_callbacks, false, false, E_SCAN_TYPE_ON_DEMAND);
    auto result = s.scan("/etc/passwd");

    EXPECT_FALSE(result.getDetections().empty());
    EXPECT_EQ(result.getDetections()[0].name, THREAT);
}

TEST(TestScanClient, TestFailedToOpenMessages)
{
    EXPECT_EQ(ScanClient::failedToOpen(EACCES), "Failed to open as permission denied: ");
    EXPECT_EQ(ScanClient::failedToOpen(ENAMETOOLONG), "Failed to open as the path is too long: ");
    EXPECT_EQ(ScanClient::failedToOpen(ELOOP), "Failed to open as couldn't follow the symlink further: ");
    EXPECT_EQ(ScanClient::failedToOpen(ENODEV), "Failed to open as path is a device special file and no corresponding device exists: ");
    EXPECT_EQ(ScanClient::failedToOpen(ENOENT), "Failed to open as path is a dangling symlink or a directory component is missing: ");
    EXPECT_EQ(ScanClient::failedToOpen(ENOMEM), "Failed to open due to a lack of kernel memory: ");
    EXPECT_EQ(ScanClient::failedToOpen(EOVERFLOW), "Failed to open as the file is too large: ");
    EXPECT_EQ(ScanClient::failedToOpen(2021), "Failed to open with error 2021: ");
}
