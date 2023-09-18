// Copyright 2020-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "UnixSocketMemoryAppenderUsingTests.h"
#include "ScanResponse.capnp.h"

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/SocketUtils.h"

#include "common/ApplicationPaths.h"
#include "common/FailedToInitializeSusiException.h"
#include "common/ShuttingDownException.h"
#include "datatypes/sophos_filesystem.h"
#include "scan_messages/ScanRequest.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/MockScanner.h"
#include "tests/common/TestFile.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <capnp/serialize.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;
using namespace ::testing;

namespace fs = sophos_filesystem;

namespace
{
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
            testDir_ = fs::temp_directory_path();
            testDir_ /= test_info->test_case_name();
            testDir_ /= test_info->name();
            fs::remove_all(testDir_);
            fs::create_directories(testDir_ / "chroot/var");
            fs::current_path(testDir_);

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, testDir_ );
            appConfig.setData("PLUGIN_INSTALL", testDir_ );

            sysCalls_ = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            mockSysCalls_ = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            mockFileSystem_ = std::make_unique<StrictMock<MockFileSystem>>();
            threatDetectorUnhealthyFlagFile_ = Plugin::getThreatDetectorUnhealthyFlagPath();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(testDir_);
        }

        void dontExpectBadHealth()
        {
            EXPECT_CALL(*mockFileSystem_, writeFile(threatDetectorUnhealthyFlagFile_, "")).Times(0);
            scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));
        }

        UsingMemoryAppender memoryAppenderHolder;
        scan_messages::ClientScanRequest request;
        fs::path testDir_;
        std::shared_ptr<Common::SystemCallWrapper::SystemCallWrapper> sysCalls_;
        std::shared_ptr<MockSystemCallWrapper> mockSysCalls_;
        std::unique_ptr<MockFileSystem> mockFileSystem_;
        std::string threatDetectorUnhealthyFlagFile_;
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem_;
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
            serverFd_.reset(socket_fds[0]);
            clientFd_.reset(socket_fds[1]);
            ASSERT_GE(serverFd_.get(), 0);
            ASSERT_GE(clientFd_.get(), 0);
        }

        bool receiveResponse(scan_messages::ScanResponse& response)
        {
            if (!clientFd_.valid())
            {
                return false;
            }
            auto length = readLength(clientFd_);
            if (length < 0)
            {
                return false;
            }

            size_t buffer_size = 1 + length / sizeof(capnp::word);
            auto proto_buffer = kj::heapArray<capnp::word>(buffer_size);

            auto bytes_read = ::read(clientFd_, proto_buffer.begin(), length);

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

        datatypes::AutoFd serverFd_;
        datatypes::AutoFd clientFd_;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> mockSysCalls_;
    };

    class TestScanningServerConnectionThreadWithSocketConnection : public TestScanningServerConnectionThreadWithSocketPair
    {
    protected:
        void SetUp() override
        {
            TestScanningServerConnectionThreadWithSocketPair::SetUp();

            auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
            connectionThread_ = std::make_shared<ScanningServerConnectionThread>(serverFd_, scannerFactory, sysCalls_);
        }

        std::shared_ptr<ScanningServerConnectionThread> connectionThread_;
    };
}

TEST_F(TestScanningServerConnectionThread, successful_construction)
{
    dontExpectBadHealth();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(unixsocket::ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, sysCalls_));
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_bad_fd)
{
    dontExpectBadHealth();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, scannerFactory, sysCalls_), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_null_factory)
{
    dontExpectBadHealth();

    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, nullptr, sysCalls_), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, stop_while_running)
{
    const std::string expected = "Closing ScanningServerConnectionThread";

    dontExpectBadHealth();

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, sysCalls_);
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
    const std::string expected = "ScanningServerConnectionThread closed: EOF";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, send_zero_length)
{
    const std::string expected = "ScanningServerConnectionThread ignoring length of zero / No new messages";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, sysCalls_, 1);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, bad_notify_pipe_fd)
{
    const std::string expected = "Closing ScanningServerConnectionThread, error from notify pipe";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[1].revents = POLLERR;
    EXPECT_CALL(*mockSysCalls_, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, mockSysCalls_);
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
    const std::string expected = "Closing ScanningServerConnectionThread, error from socket";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[0].revents = POLLERR;
    EXPECT_CALL(*mockSysCalls_, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, mockSysCalls_);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, over_max_length)
{
    const std::string expected = "Aborting ScanningServerConnectionThread: failed to read length";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd, 0x1000080);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, max_length)
{
    const std::string expected = "ScanningServerConnectionThread: Aborting: failed to read entire message";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, sysCalls_);
    connectionThread.readTimeout_ = std::chrono::milliseconds{1};
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // length is limited to ~16MB
    unixsocket::writeLength(clientFd, 0x100007f);
    ::close(clientFd);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, corrupt_request)
{
    const std::string expected = "Terminated ScanningServerConnectionThread with serialisation exception: ";
    const std::string request = { 0x01, 0x00 };

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, valid_request_no_fd)
{
    const std::string expected = "Aborting ScanningServerConnectionThread: failed to read fd";

    dontExpectBadHealth();
    connectionThread_->start();
    EXPECT_TRUE(connectionThread_->isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());
    ::close(clientFd_);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread_->requestStop();
    connectionThread_->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketPair, send_fd)
{
    const std::string expected = "ScanningServerConnectionThread managed to get file descriptor: ";

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();
    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "virus", "THREAT", "");

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::FileScanRequest>();
    requestBuilder.setPathname("/file/to/scan");
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    requestBuilder.setUserID("n/a");

    Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

    EXPECT_CALL(*mockFileSystem_, removeFile(threatDetectorUnhealthyFlagFile_, true)).Times(1);
    scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));

    scan_messages::ScanRequest request = scan_messages::ScanRequest(requestReader);
    EXPECT_CALL(*scanner, scan(_, Eq(std::ref(request)))).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false, false)).WillOnce(Return(ByMove(std::move(scanner))));

    ScanningServerConnectionThread connectionThread(serverFd_, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());

    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open());
    auto ret = send_fd(clientFd_, fd.get()); // send a valid file descriptor
    ASSERT_GE(ret, 0);

    auto length = unixsocket::readLength(clientFd_);
    static_cast<void>(length);

    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketPair, fails_to_create_scanner_throws_and_sets_unhealthy_flag)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));
    const std::string throwstr = "imbroken";
    const std::string expected = "ScanningServerConnectionThread aborting scan, failed to initialise SUSI: " + throwstr;

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::FileScanRequest>();
    requestBuilder.setPathname("/file/to/scan");
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    requestBuilder.setUserID("n/a");

    Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

    scan_messages::ScanRequest request = scan_messages::ScanRequest(requestReader);
    EXPECT_CALL(*scannerFactory, createScanner(_, _, _)).WillOnce(Throw(FailedToInitializeSusiException(throwstr)));

    EXPECT_CALL(*mockFileSystem_, writeFile(threatDetectorUnhealthyFlagFile_, "")).Times(1);
    scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));

    ScanningServerConnectionThread connectionThread(serverFd_, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());

    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open());
    auto ret = send_fd(clientFd_, fd.get()); // send a valid file descriptor
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog("[ERROR] " + expected));
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    EXPECT_TRUE(waitForLog("Stopping ScanningServerConnectionThread"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestScanningServerConnectionThreadWithSocketPair, successful_creation_of_scanner_removes_bad_health_flag)
{
    auto scanner = std::make_unique<StrictMock<MockScanner>>();
    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "virus", "THREAT", "");
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::FileScanRequest>();
    requestBuilder.setPathname("/file/to/scan");
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    requestBuilder.setUserID("n/a");

    EXPECT_CALL(*mockFileSystem_, removeFile(threatDetectorUnhealthyFlagFile_, true)).Times(1);
    scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));

    Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;
    scan_messages::ScanRequest request = scan_messages::ScanRequest(requestReader);
    EXPECT_CALL(*scanner, scan(_, Eq(std::ref(request)))).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(_, _, _)).WillOnce(Return(ByMove(std::move(scanner))));

    ScanningServerConnectionThread connectionThread(serverFd_, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());

    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open());
    auto ret = send_fd(clientFd_, fd.get()); // send a valid file descriptor
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog("ScanningServerConnectionThread has created a new scanner"));

    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestScanningServerConnectionThreadWithSocketPair, scanner_shutting_down_throws)
{
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));
    const std::string expected = "ScanningServerConnectionThread aborting scan, scanner is shutting down";

    dontExpectBadHealth();
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
        message.initRoot<Sophos::ssplav::FileScanRequest>();
    requestBuilder.setPathname("/file/to/scan");
    requestBuilder.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    requestBuilder.setUserID("n/a");

    Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

    scan_messages::ScanRequest request = scan_messages::ScanRequest(requestReader);
    EXPECT_CALL(*scannerFactory, createScanner(_, _, _)).WillOnce(Throw(ShuttingDownException()));

    ScanningServerConnectionThread connectionThread(serverFd_, scannerFactory, sysCalls_);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());

    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open());
    auto ret = send_fd(clientFd_, fd.get()); // send a valid file descriptor
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog("[ERROR] " + expected));
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);
    EXPECT_TRUE(waitForLog("Stopping ScanningServerConnectionThread"));

    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_not_readable)
{
    const std::string expected = "Received file FD is not open for read";

    dontExpectBadHealth();
    connectionThread_->start();
    EXPECT_TRUE(connectionThread_->isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());
    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open(O_WRONLY));
    auto ret = send_fd(clientFd_, fd.get());
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog(expected));
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    connectionThread_->requestStop();
    connectionThread_->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_is_device)
{
    const std::string expected = "Received file FD is not a regular file";

    dontExpectBadHealth();
    connectionThread_->start();
    EXPECT_TRUE(connectionThread_->isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());
    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    ASSERT_GE(devNull.get(), 0);
    auto ret = send_fd(clientFd_, devNull.get());
    devNull.close();
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog(expected));
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    connectionThread_->requestStop();
    connectionThread_->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_is_socket)
{
    const std::string expected = "Received file FD is not a regular file";

    dontExpectBadHealth();
    connectionThread_->start();
    EXPECT_TRUE(connectionThread_->isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());
    auto ret = send_fd(clientFd_, clientFd_.get());
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog(expected));
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    connectionThread_->requestStop();
    connectionThread_->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThreadWithSocketConnection, fd_is_path)
{
    const std::string expected = "Received file FD is not open for read";

    dontExpectBadHealth();
    connectionThread_->start();
    EXPECT_TRUE(connectionThread_->isRunning());
    unixsocket::writeLengthAndBuffer(clientFd_, request.serialise());
    TestFile testFile("testfile");
    datatypes::AutoFd fd(testFile.open(O_PATH));
    ASSERT_GE(fd.get(), 0);
    auto ret = send_fd(clientFd_, fd.get());
    fd.close();
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(waitForLog(expected));
    scan_messages::ScanResponse response;
    receiveResponse(response);
    EXPECT_EQ(response.getErrorMsg(), expected);

    connectionThread_->requestStop();
    connectionThread_->join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}
