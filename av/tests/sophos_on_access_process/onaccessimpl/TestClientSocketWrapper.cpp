// Copyright 2022, Sophos Limited.  All rights reserved.

#include "common/RecordingMockSocket.h"
#include "../SoapMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/onaccessimpl/ClientSocketWrapper.h"
#include "sophos_on_access_process/onaccessimpl/ReconnectSettings.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;
using namespace std::chrono_literals;

namespace
{
    class TestClientSocketWrapper : public SoapMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override {}

        void TearDown() override {}
    };

    class MockIScanningClientSocket : public unixsocket::IScanningClientSocket
    {
    public:
        MockIScanningClientSocket()
        {
            m_socketFd.reset(::open("/dev/zero", O_RDONLY));
            ON_CALL(*this, connect).WillByDefault(Return(0));
            ON_CALL(*this, sendRequest).WillByDefault(Return(true));
            ON_CALL(*this, receiveResponse).WillByDefault(Return(true));
            ON_CALL(*this, socketFd).WillByDefault([this]() {return this->m_socketFd.fd();});
        }

        MOCK_METHOD(int, connect, ());
        MOCK_METHOD(bool, sendRequest, (scan_messages::ClientScanRequestPtr request));
        MOCK_METHOD(bool, receiveResponse, (scan_messages::ScanResponse& response));
        MOCK_METHOD(int, socketFd, ());

    private:
        datatypes::AutoFd m_socketFd;
    };
}

TEST_F(TestClientSocketWrapper, Construction)
{
    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);

    ClientSocketWrapper csw {socket, notifyPipe};
}

TEST_F(TestClientSocketWrapper, Scan)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe};

    {
        InSequence seq;

        EXPECT_CALL(socket, sendRequest(_))
            .Times(1);
        EXPECT_CALL(socket, socketFd)
            .Times(1);
        EXPECT_CALL(socket, receiveResponse(_))
            .Times(1)
            .WillOnce(testing::DoAll(
                testing::SetArgReferee<0>(response),
                Return(true)
                    ));
    }

    auto gotResponse = csw.scan(request);
    EXPECT_TRUE(response.allClean());

    auto detections = response.getDetections();
    ASSERT_EQ(detections.size(), 0);
}

TEST_F(TestClientSocketWrapper, RetriesConnect)
{
    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};


    InSequence seq;
    EXPECT_CALL(socket, connect())
        .Times(1)
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(socket, connect())
        .Times(1)
        .WillOnce(Return(0));

    ClientSocketWrapper csw {socket, notifyPipe, 1ms };
}

TEST_F(TestClientSocketWrapper, ConnectRetryLimit)
{
    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    InSequence seq;
    EXPECT_CALL(socket, connect())
        .Times(MAX_CONN_RETRIES)
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(socket, connect())
        .Times(0);

    ClientSocketWrapper csw {socket, notifyPipe, 1ms };
}

TEST_F(TestClientSocketWrapper, ScanRetriesSend)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, 1ms};

    {
        InSequence seq;

        EXPECT_CALL(socket, sendRequest(_))
            .WillOnce(Return(false));
        EXPECT_CALL(socket, connect)
            .Times(1);

        EXPECT_CALL(socket, sendRequest(_))
            .WillOnce(Return(true));
        EXPECT_CALL(socket, socketFd)
            .Times(1);
        EXPECT_CALL(socket, receiveResponse(_))
            .Times(1)
            .WillOnce(testing::DoAll(
                testing::SetArgReferee<0>(response),
                Return(true)
                    ));
    }

    auto gotResponse = csw.scan(request);
    EXPECT_TRUE(response.allClean());
}

TEST_F(TestClientSocketWrapper, ScanRetryLimit)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    request->setPath("/foo/bar");
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, 1ms};

    {
        InSequence seq;

        for (int i=0; i<MAX_SCAN_RETRIES; ++i)
        {
            EXPECT_CALL(socket, sendRequest(_)).WillOnce(Return(false));
            EXPECT_CALL(socket, connect).Times(1);
        }

        EXPECT_CALL(socket, sendRequest(_))
            .Times(0);
    }

    auto gotResponse = csw.scan(request);
    auto errMsg = gotResponse.getErrorMsg();
    EXPECT_EQ(errMsg, "Failed to scan file: /foo/bar after 60 retries");
}

TEST_F(TestClientSocketWrapper, ScanReconnectLimit)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    request->setPath("/foo/bar");
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, 1ms};

    {
        InSequence seq;

        for (int i=0; i<TOTAL_MAX_RECONNECTS; ++i)
        {
            EXPECT_CALL(socket, sendRequest(_)).WillOnce(Return(false));
            EXPECT_CALL(socket, connect).Times(1);
        }

        EXPECT_CALL(socket, sendRequest(_))
            .Times(0);
    }

    for (int i=0; i< TOTAL_MAX_RECONNECTS/MAX_SCAN_RETRIES; ++i)
    {
        auto gotResponse = csw.scan(request);
        auto errMsg = gotResponse.getErrorMsg();
        EXPECT_EQ(errMsg, "Failed to scan file: /foo/bar after 60 retries");
    }

    EXPECT_THROW(csw.scan(request), common::AbortScanException);
}

TEST_F(TestClientSocketWrapper, ScanRetriesReceive)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, 1ms};

    {
        InSequence seq;

        EXPECT_CALL(socket, sendRequest(_))
            .WillOnce(Return(true));
        EXPECT_CALL(socket, socketFd)
            .Times(1);
        EXPECT_CALL(socket, receiveResponse(_))
            .Times(1)
            .WillOnce(Return(false));
        EXPECT_CALL(socket, connect)
            .Times(1);

        EXPECT_CALL(socket, sendRequest(_))
            .WillOnce(Return(true));
        EXPECT_CALL(socket, socketFd)
            .Times(1);
        EXPECT_CALL(socket, receiveResponse(_))
            .Times(1)
            .WillOnce(testing::DoAll(
                testing::SetArgReferee<0>(response),
                Return(true)
                    ));
    }

    auto gotResponse = csw.scan(request);
    EXPECT_TRUE(response.allClean());

    auto detections = response.getDetections();
    ASSERT_EQ(detections.size(), 0);
}
