// Copyright 2022 Sophos Limited. All rights reserved.

#include "FakeScanningServer.h"

#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/SocketUtils.h"

#include "../UnixSocketMemoryAppenderUsingTests.h"

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

    std::ofstream ofs{socketPath};

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

TEST_F(TestScanningClientSocket, send_scan_request_and_receive_response)
{
    fs::path socketPath = m_testDir / "socket";
    fs::path scannedFilePath = m_testDir / "testFile";

    std::ofstream ofstr{scannedFilePath};
    ofstr << "This is a file";
    ofstr.close();

    FakeScanningServer serverSocket{socketPath};
    serverSocket.start();

    std::ofstream ofs{socketPath};

    datatypes::AutoFd fileFd{::open(scannedFilePath.c_str(), O_RDONLY)};
    ASSERT_TRUE(fileFd.valid());

    {
        unixsocket::ScanningClientSocket socket{ socketPath };
        int result = socket.connect();
        ASSERT_EQ(result, 0);
        scan_messages::ClientScanRequestPtr request = std::make_shared<scan_messages::ClientScanRequest>(
            nullptr,
            fileFd
            );
        request->setPath(scannedFilePath);
        bool sent = socket.sendRequest(request);
        ASSERT_TRUE(sent);
        scan_messages::ScanResponse response;
        bool received = socket.receiveResponse(response);
        ASSERT_TRUE(received);
    }

    serverSocket.tryStop();
    serverSocket.join();
}

