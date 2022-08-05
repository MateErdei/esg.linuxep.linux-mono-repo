/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
        MOCK_METHOD(bool, sendRequest, (datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request));
        MOCK_METHOD(bool, receiveResponse, (scan_messages::ScanResponse& response));
        MOCK_METHOD(int, socketFd, ());

    private:
        datatypes::AutoFd m_socketFd;
    };
}

TEST_F(TestClientSocketWrapper, Construction)
{
    MockIScanningClientSocket socket {};

    EXPECT_CALL(socket, connect).Times(1);

    ClientSocketWrapper csw {socket};
}

TEST_F(TestClientSocketWrapper, Scan)
{
    datatypes::AutoFd fd {};

    scan_messages::ClientScanRequest request;
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket};

    {
        InSequence seq;

        EXPECT_CALL(socket, sendRequest(_, _))
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

    auto gotResponse = csw.scan(fd, request);
    EXPECT_TRUE(response.allClean());

    auto detections = response.getDetections();
    ASSERT_EQ(detections.size(), 0);
}

TEST_F(TestClientSocketWrapper, ConnectFails)
{
    MockIScanningClientSocket socket {};

    EXPECT_CALL(socket, connect())
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW(ClientSocketWrapper csw {socket}, ClientSocketException);
}

TEST_F(TestClientSocketWrapper, SendFails)
{
    datatypes::AutoFd fd {};

    scan_messages::ClientScanRequest request;
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket};

    EXPECT_CALL(socket, sendRequest(_, _))
            .WillOnce(Return(false));

    EXPECT_THROW(csw.scan(fd, request), ClientSocketException);
}

TEST_F(TestClientSocketWrapper, ReceiveFails)
{
    datatypes::AutoFd fd {};

    scan_messages::ClientScanRequest request;
    scan_messages::ScanResponse response;

    MockIScanningClientSocket socket {};

    EXPECT_CALL(socket, connect).Times(1);
    ClientSocketWrapper csw {socket};

    {
        InSequence seq;

        EXPECT_CALL(socket, sendRequest(_, _))
            .WillOnce(Return(true));
        EXPECT_CALL(socket, socketFd)
            .Times(1);
        EXPECT_CALL(socket, receiveResponse(_))
            .Times(1)
            .WillOnce(Return(false));
    }

    EXPECT_THROW(csw.scan(fd, request), ClientSocketException);
}
