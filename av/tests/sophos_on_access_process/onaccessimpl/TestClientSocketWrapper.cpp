// Copyright 2022-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "common/RecordingMockSocket.h"
#include "../SoapMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/onaccessimpl/ClientSocketWrapper.h"
#include "sophos_on_access_process/onaccessimpl/ReconnectSettings.h"
#include "tests/unixsocket/MockIScanningClientSocket.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;
using namespace std::chrono_literals;

namespace
{
    class TestOnAccessClientSocketWrapper : public SoapMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override {}

        void TearDown() override {}
        using ScanRequest_t = scan_messages::ClientScanRequest;
        using ScanRequestPtr = scan_messages::ClientScanRequestPtr;

        static ScanRequestPtr emptyRequest()
        {
            return std::make_shared<ScanRequest_t>();
        }
    };

    const struct timespec& oneMillisecond = { 0, 1000000 }; // 1ms
}

TEST_F(TestOnAccessClientSocketWrapper, Construction)
{
    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);

    ClientSocketWrapper csw {socket, notifyPipe};
}

TEST_F(TestOnAccessClientSocketWrapper, Scan)
{
    auto request = emptyRequest();
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

TEST_F(TestOnAccessClientSocketWrapper, RetriesConnect)
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

    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};
}

TEST_F(TestOnAccessClientSocketWrapper, ConnectRetryLimit)
{
    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    InSequence seq;
    EXPECT_CALL(socket, connect())
        .Times(MAX_CONN_RETRIES)
        .WillRepeatedly(Return(-1));
    EXPECT_CALL(socket, connect())
        .Times(0);

    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};
}

TEST_F(TestOnAccessClientSocketWrapper, ScanRetriesSend)
{
    auto request = emptyRequest();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};

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

TEST_F(TestOnAccessClientSocketWrapper, ScanRetryLimit)
{
    auto request = emptyRequest();
    request->setPath("/foo/bar");
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};

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

TEST_F(TestOnAccessClientSocketWrapper, SendRequestSuccessfulButNoResponseReceived)
{
    auto request = emptyRequest();
    request->setPath("/foo/bar");

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).WillRepeatedly(Return(0));
    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};

    EXPECT_CALL(socket, sendRequest(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(socket, receiveResponse(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(socket, socketFd()).WillRepeatedly(Return(9));

    auto gotResponse = csw.scan(request);
    auto errMsg = gotResponse.getErrorMsg();
    // number in msg should correspond to "MAX_SCAN_RETRIES"
    EXPECT_EQ(errMsg, "Failed to scan file: /foo/bar after 60 retries");
}

TEST_F(TestOnAccessClientSocketWrapper, ScanReconnectLimit)
{
    auto request = emptyRequest();
    request->setPath("/foo/bar");
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};

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

TEST_F(TestOnAccessClientSocketWrapper, ScanRetriesReceive)
{
    auto request = emptyRequest();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe, oneMillisecond};

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
