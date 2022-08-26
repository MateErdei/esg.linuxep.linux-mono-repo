// Copyright 2022, Sophos Limited.  All rights reserved.

#include "common/RecordingMockSocket.h"
#include "../SoapMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/onaccessimpl/ClientSocketWrapper.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;

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

TEST_F(TestClientSocketWrapper, ConnectFails)
{
    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect())
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW(ClientSocketWrapper csw(socket, notifyPipe), ClientSocketException);
}

TEST_F(TestClientSocketWrapper, SendFails)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe};

    EXPECT_CALL(socket, sendRequest(_))
            .WillOnce(Return(false));

    EXPECT_THROW(csw.scan(request), ClientSocketException);
}

TEST_F(TestClientSocketWrapper, ReceiveFails)
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
            .WillOnce(Return(true));
        EXPECT_CALL(socket, socketFd)
            .Times(1);
        EXPECT_CALL(socket, receiveResponse(_))
            .Times(1)
            .WillOnce(Return(false));
    }

    EXPECT_THROW(csw.scan(request), ClientSocketException);
}

TEST_F(TestClientSocketWrapper, ScanTerminates)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};
    Common::Threads::NotifyPipe notifyPipe {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket, notifyPipe};

    notifyPipe.notify();

    {
        InSequence seq;

        EXPECT_CALL(socket, sendRequest(_))
            .WillOnce(Return(true));
        EXPECT_CALL(socket, socketFd)
            .Times(1);
    }
    EXPECT_CALL(socket, receiveResponse(_))
        .Times(0);

    EXPECT_THROW(csw.scan(request), ScanInterruptedException);
}