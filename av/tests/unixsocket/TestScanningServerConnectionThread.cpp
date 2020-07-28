/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "tests/common/MemoryAppender.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <ctime>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace unixsocket;

namespace
{
    class TestScanningServerConnectionThread : public MemoryAppenderUsingTests
    {
    public:
        TestScanningServerConnectionThread()
            : MemoryAppenderUsingTests("UnixSocket")
        {}

    };

    class MockScanner : public threat_scanner::IThreatScanner
    {
    public:
        MOCK_METHOD4(scan, scan_messages::ScanResponse(datatypes::AutoFd&, const std::string&, int64_t,
            const std::string& userID));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD1(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives));
    };

}

using namespace ::testing;

TEST_F(TestScanningServerConnectionThread, successful_construction) //NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(unixsocket::ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory));
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_bad_fd) //NOLINT
{
    using namespace unixsocket;
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, scannerFactory), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_null_factory) //NOLINT
{
    using namespace unixsocket;
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, nullptr), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, stop_while_running) //NOLINT
{
    const std::string expected = "Closing scanning socket thread";
    setupMemoryAppender();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    connectionThread.start();
    // No waiting...
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appender_contains(expected));
    clearMemoryAppender();
}

TEST_F(TestScanningServerConnectionThread, eof_while_running) //NOLINT
{
    const std::string expected = "Scanning Server Connection closed: EOF";
    setupMemoryAppender();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    connectionThread.start();
    wait_for_log(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appender_contains(expected));
    clearMemoryAppender();
}

TEST_F(TestScanningServerConnectionThread, send_zero_length) //NOLINT
{
    const std::string expected = "Ignoring length of zero / No new messages";
    setupMemoryAppender();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    connectionThread.start();
    wait_for_log(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appender_contains(expected));
    clearMemoryAppender();
}

TEST_F(TestScanningServerConnectionThread, closed_fd) //NOLINT
{
    const std::string expected = "Socket failed: 9";
    setupMemoryAppender();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    wait_for_log(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appender_contains(expected));
    clearMemoryAppender();
}