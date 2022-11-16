/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"
#include <ScanResponse.capnp.h>

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/SocketUtils.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/TestFile.h"
#include "tests/datatypes/MockSysCalls.h"
#include <capnp/serialize.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;
using namespace ::testing;

namespace fs = sophos_filesystem;

namespace
{

    class MockScanner : public threat_scanner::IThreatScanner
    {
    public:
        MOCK_METHOD4(scan, scan_messages::ScanResponse(datatypes::AutoFd&, const std::string&, int64_t,
            const std::string& userID));

        MOCK_METHOD2(susiErrorToReadableError, std::string(const std::string& filePath, const std::string& susiError));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD2(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives, bool scanImages));

        MOCK_METHOD0(update, bool());
        MOCK_METHOD0(reload, bool());
        MOCK_METHOD0(shutdown, void());
        MOCK_METHOD0(susiIsInitialized, bool());
    };

    class TestScanningServerConnectionThread : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
        TestScanningServerConnectionThread()
            : memoryAppenderHolder(*this)
        {
            request.setPath("/file/to/scan");
            request.setScanInsideArchives(false);
            request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
            request.setUserID("root");
        }

        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            fs::current_path(m_testDir);

            m_sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        UsingMemoryAppender memoryAppenderHolder;
        scan_messages::ClientScanRequest request;
        fs::path m_testDir;
        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCalls;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
    };

    class TestScanningServerConnectionThreadWithSocketPair : public TestScanningServerConnectionThread
    {
    protected:
        void SetUp() override
        {
            TestScanningServerConnectionThread::SetUp();

            int socket_fds[2];
            int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
            ASSERT_EQ(ret, 0);
            m_serverFd.reset(socket_fds[0]);
            m_clientFd.reset(socket_fds[1]);
            ASSERT_GE(m_serverFd.get(), 0);
            ASSERT_GE(m_clientFd.get(), 0);

            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        bool receiveResponse(scan_messages::ScanResponse& response)
        {
            if (!m_clientFd.valid())
            {
                return false;
            }
            auto length = readLength(m_clientFd);
            if (length < 0)
            {
                return false;
            }

            size_t buffer_size = 1 + length / sizeof(capnp::word);
            auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

            auto bytes_read = ::read(m_clientFd, proto_buffer.begin(), length);

            if (bytes_read != length)
            {
                return false;
            }

            auto view = proto_buffer.slice(0, bytes_read / sizeof(capnp::word));

            try
            {
                capnp::FlatArrayMessageReader messageInput(view);
                Sophos::ssplav::FileScanResponse::Reader responseReader =
                    messageInput.getRoot<Sophos::ssplav::FileScanResponse>();

                response = scan_messages::ScanResponse(responseReader);
            }
            catch (kj::Exception& ex)
            {
                if (ex.getType() == kj::Exception::Type::UNIMPLEMENTED)
                {
                    // Fatal since this means we have a coding error that calls something unimplemented in kj.
                    PRINT(
                        "Terminated ScanningClientSocket with serialisation unimplemented exception: "
                        << ex.getDescription().cStr());
                }
                else
                {
                    PRINT(
                        "Terminated ScanningClientSocket with serialisation exception: " << ex.getDescription().cStr());
                }

                std::stringstream errorMsg;
                errorMsg << "Malformed response from Sophos Threat Detector (" << ex.getDescription().cStr() << ")";
                throw std::runtime_error(errorMsg.str());
            }

            return true;
        }

        datatypes::AutoFd m_serverFd;
        datatypes::AutoFd m_clientFd;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
    };

    class TestScanningServerConnectionThreadWithSocketConnection : public TestScanningServerConnectionThreadWithSocketPair
    {
    protected:
        void SetUp() override
        {
            TestScanningServerConnectionThreadWithSocketPair::SetUp();

            auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
            m_connectionThread = std::make_shared<ScanningServerConnectionThread>(m_serverFd, scannerFactory, m_sysCalls);
        }

        std::shared_ptr<ScanningServerConnectionThread> m_connectionThread;
    };
}

TEST_F(TestScanningServerConnectionThread, successful_construction)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(unixsocket::ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCalls));
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_bad_fd)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, scannerFactory, m_sysCalls), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_null_factory)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, nullptr, m_sysCalls), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, stop_while_running)
{
    const std::string expected = "Closing Scanning connection thread";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCalls);
    EXPECT_FALSE(connectionThread.isRunning());
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // No waiting...
    connectionThread.requestStop();
    connectionThread.join();
    EXPECT_FALSE(connectionThread.isRunning());

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, eof_while_running)
{
    const std::string expected = "Scanning connection thread closed: EOF";


    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCalls);
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, send_zero_length)
{
    const std::string expected = "Ignoring length of zero / No new messages";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCalls, 1);
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, bad_notify_pipe_fd)
{
    const std::string expected = "Closing Scanning Server connection thread, error from notify pipe";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[1].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, bad_socket_fd)
{
    const std::string expected = "Closing Scanning Server connection thread, error from socket";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[0].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, over_max_length)
{
    const std::string expected = "Aborting Scanning connection thread: failed to read length";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd, 0x1000080);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, max_length)
{
    const std::string expected = "Aborting Scanning connection thread: failed to read entire message";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // length is limited to ~16MB
    unixsocket::writeLength(clientFd, 0x100007f);
    ::close(clientFd);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, corrupt_request)
{
    const std::string expected = "Terminated ScanningServerConnectionThread with serialisation exception: ";

    const std::string request = { 0x01, 0x00 };

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, valid_request_no_fd)
{
    const std::string expected = "Aborting Scanning connection thread: failed to read fd";

    m_connectionThread->start();
    EXPECT_TRUE(m_connectionThread->isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd, request.serialise());
    ::close(m_clientFd);
    waitForLog(expected);
    m_connectionThread->requestStop();
    m_connectionThread->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketPair, send_fd) // NOLINT
{
    const std::string expected = "Managed to get file descriptor: ";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();
    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "THREAT","");

    EXPECT_CALL(*scanner, scan(_, "/file/to/scan", _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));

    ScanningServerConnectionThread connectionThread(m_serverFd, scannerFactory, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd, request.serialise());

    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open());
    int ret = send_fd(m_clientFd, fd.get()); // send a valid file descriptor
    ASSERT_GE(ret, 0);

    int length = unixsocket::readLength(m_clientFd);
    static_cast<void>(length);

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_not_readable)
{
    const std::string expected = "Received file FD is not open for read";

    m_connectionThread->start();
    EXPECT_TRUE(m_connectionThread->isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd, request.serialise());
    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open(O_WRONLY));
    int ret = send_fd(m_clientFd, fd.get());
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    m_connectionThread->requestStop();
    m_connectionThread->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_is_device)
{
    const std::string expected = "Received file FD is not a regular file";

    m_connectionThread->start();
    EXPECT_TRUE(m_connectionThread->isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd, request.serialise());
    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    ASSERT_GE(devNull.get(), 0);
    int ret = send_fd(m_clientFd, devNull.get());
    devNull.close();
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    m_connectionThread->requestStop();
    m_connectionThread->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_is_socket)
{
    const std::string expected = "Received file FD is not a regular file";

    m_connectionThread->start();
    EXPECT_TRUE(m_connectionThread->isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd, request.serialise());
    int ret = send_fd(m_clientFd, m_clientFd.get());
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    m_connectionThread->requestStop();
    m_connectionThread->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_is_path)
{
    const std::string expected = "Received file FD is not open for read";

    m_connectionThread->start();
    EXPECT_TRUE(m_connectionThread->isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd, request.serialise());
    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open(O_PATH));
    ASSERT_GE(fd.get(), 0);
    int ret = send_fd(m_clientFd, fd.get());
    fd.close();
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    m_connectionThread->requestStop();
    m_connectionThread->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}
