// Copyright 2022-2023 Sophos Limited. All rights reserved.

// Product code
#include "common/AbortScanException.h"
#include "common/ThreadRunner.h"
#include "unixsocket/Logger.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
// Test code
#include "FakeScanningServer.h"

#include "../UnixSocketMemoryAppenderUsingTests.h"
#include "common/SaferStrerror.h"

#include <gtest/gtest.h>

#include <fstream>

using namespace ::testing;
namespace fs = sophos_filesystem;

namespace
{
    class TestScanningClientSocket : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_testDir = createTestSpecificDirectory();
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }
        fs::path m_testDir;
    };
}

TEST_F(TestScanningClientSocket, construction)
{
    EXPECT_NO_THROW(unixsocket::ScanningClientSocket socket{m_testDir / "socket"});
}

TEST_F(TestScanningClientSocket, connect_to_non_existent_socket)
{
    unixsocket::ScanningClientSocket socket{m_testDir / "socket"};
    errno = 0;
    int result = socket.connect();
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, ENOENT);
}

TEST_F(TestScanningClientSocket, connect_to_file)
{
    fs::path socketPath = m_testDir / "socket";

    std::ofstream ofs{socketPath};

    unixsocket::ScanningClientSocket socket{socketPath};
    errno = 0;
    int result = socket.connect();
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, ECONNREFUSED);
}

TEST_F(TestScanningClientSocket, connect_to_socket)
{
    fs::path socketPath = m_testDir / "socket";

    FakeScanningServer serverSocket{socketPath};
    serverSocket.start();

    {
        unixsocket::ScanningClientSocket socket{ socketPath };
        errno = 0;
        int result = socket.connect();
        EXPECT_EQ(result, 0);
        EXPECT_EQ(errno, 0);
    }

    serverSocket.tryStop();
    serverSocket.join();
}
namespace
{
    scan_messages::ClientScanRequestPtr make_request(datatypes::AutoFd& fileFd, const fs::path& scannedFilePath)
    {
        scan_messages::ClientScanRequestPtr request = std::make_shared<scan_messages::ClientScanRequest>(
            nullptr,
            fileFd
        );
        request->setPath(scannedFilePath);
        return request;
    }


    scan_messages::ScanResponse make_response()
    {
        return {};
    }
}

TEST_F(TestScanningClientSocket, send_scan_request_and_receive_response)
{
    fs::path socketPath = m_testDir / "socket";
    fs::path scannedFilePath = m_testDir / "testFile";

    std::ofstream ofstr{scannedFilePath};
    ofstr << "This is a file";
    ofstr.close();

    auto serverSocket = std::make_shared<FakeScanningServer>(socketPath);
    auto expected_response = make_response();
    serverSocket->m_nextResponse = expected_response.serialise();
    common::ThreadRunner serverSocketRunner{serverSocket, "fakeServerSocket", true};

    datatypes::AutoFd fileFd{::open(scannedFilePath.c_str(), O_RDONLY)};
    ASSERT_TRUE(fileFd.valid());

    auto request = make_request(fileFd, scannedFilePath);

    {
        unixsocket::ScanningClientSocket socket{ socketPath };
        int result = socket.connect();
        ASSERT_EQ(result, 0);;
        bool sent = socket.sendRequest(request);
        ASSERT_TRUE(sent);
        scan_messages::ScanResponse response;
        bool received = socket.receiveResponse(response);
        ASSERT_TRUE(received);
    }
}

TEST_F(TestScanningClientSocket, receive_request_instead_of_response)
{
    fs::path socketPath = m_testDir / "socket";
    fs::path scannedFilePath = m_testDir / "testFile";

    std::ofstream ofstr{scannedFilePath};
    ofstr << "This is a file";
    ofstr.close();

    auto serverSocket = std::make_shared<FakeScanningServer>(socketPath);
    common::ThreadRunner serverSocketRunner{serverSocket, "fakeServerSocket", true};

    datatypes::AutoFd fileFd{::open(scannedFilePath.c_str(), O_RDONLY)};
    ASSERT_TRUE(fileFd.valid());

    auto request = make_request(fileFd, scannedFilePath);

    // Send an invalid response
    auto requestSerialised = request->serialise();
    serverSocket->m_nextResponse = requestSerialised;

    {
        unixsocket::ScanningClientSocket socket{ socketPath };
        int result = socket.connect();
        ASSERT_EQ(result, 0);

        bool sent = socket.sendRequest(request);
        ASSERT_TRUE(sent);
        scan_messages::ScanResponse response;
        bool received = socket.receiveResponse(response);
        ASSERT_TRUE(received);
        PRINT("Request as response error message: " << response.getErrorMsg());
    }
}

TEST_F(TestScanningClientSocket, send_ff_instead_of_response)
{
    fs::path socketPath = m_testDir / "socket";
    fs::path scannedFilePath = m_testDir / "testFile";

    std::ofstream ofstr{scannedFilePath};
    ofstr << "This is a file";
    ofstr.close();

    auto serverSocket = std::make_shared<FakeScanningServer>(socketPath);
    common::ThreadRunner serverSocketRunner{serverSocket, "fakeServerSocket", true};

    datatypes::AutoFd fileFd{::open(scannedFilePath.c_str(), O_RDONLY)};
    ASSERT_TRUE(fileFd.valid());

    auto request = make_request(fileFd, scannedFilePath);

    // Send an invalid response
    serverSocket->m_nextResponse = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";

    {
        unixsocket::ScanningClientSocket socket{ socketPath };
        int result = socket.connect();
        ASSERT_EQ(result, 0);

        bool sent = socket.sendRequest(request);
        ASSERT_TRUE(sent);
        scan_messages::ScanResponse response;
        try
        {
            bool received = socket.receiveResponse(response);
            if (received)
            {
                FAIL() << "Able to receive response";
            }
        }
        catch (const common::AbortScanException& ex)
        {
            // expected
        }
    }
}

namespace
{

    void waitForResponse(int socket)
    {
        struct pollfd fds[] {
            { .fd = socket, .events = POLLIN, .revents = 0 },
        };

        while (true)
        {
            auto ret = ::ppoll(fds, std::size(fds), nullptr, nullptr);

            if (ret < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                LOGERROR("Error from ppoll: " << common::safer_strerror(errno));
                throw std::runtime_error("Error while waiting for scan response");
            }
            else if (ret > 0)
            {
                break;
            }
        }
    }


}

TEST_F(TestScanningClientSocket, receive_really_long_message_instead_of_response)
{
    fs::path socketPath = m_testDir / "socket";
    fs::path scannedFilePath = m_testDir / "testFile";

    std::ofstream ofstr{scannedFilePath};
    ofstr << "This is a file";
    ofstr.close();

    auto serverSocket = std::make_shared<FakeScanningServer>(socketPath);
    common::ThreadRunner serverSocketRunner{serverSocket, "fakeServerSocket", true};

    datatypes::AutoFd fileFd{::open(scannedFilePath.c_str(), O_RDONLY)};
    ASSERT_TRUE(fileFd.valid());

    auto request = make_request(fileFd, scannedFilePath);

    // Send a giant response
    serverSocket->m_sendGiantResponse = true;

    {
        unixsocket::ScanningClientSocket socket{ socketPath };
        int result = socket.connect();
        ASSERT_EQ(result, 0);

        bool sent = socket.sendRequest(request);
        ASSERT_TRUE(sent);
        waitForResponse(socket.socketFd());

        scan_messages::ScanResponse response;
        bool received = socket.receiveResponse(response);
        if (received)
        {
            FAIL() << "Able to receive response";
        }
        else
        {
            PRINT("receiveResponse => false"); // message too long
        }
    }
}



