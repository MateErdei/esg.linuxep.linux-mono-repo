// Copyright 2020-2022, Sophos Limited.  All rights reserved.

// Package under test
#include "unixsocket/SocketUtils.h"
#include "unixsocket/SocketUtilsImpl.h"
// Product
#include "common/SaferStrerror.h"
#include <datatypes/AutoFd.h>
#include <datatypes/Print.h>

// Test utils
#include "UnixSocketMemoryAppenderUsingTests.h"

// 3rd party
#include <gtest/gtest.h>

// Std C++
#include <deque>
#include <utility>
// Std C
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>


using namespace unixsocket;
using namespace testing;

TEST(TestSplit, TestSplitOneByte)
{
    auto result = splitInto7Bits(0x64);
    EXPECT_THAT(result, ElementsAre(0x64));
}

TEST(TestSplit, TestSplitTwoBytes)
{
    auto result = splitInto7Bits(0xff);
    EXPECT_THAT(result, ElementsAre(0x01, 0x7f));
}

TEST(TestSplit, TestSplitTwoBytesMax)
{
    auto result = splitInto7Bits(0x3fff);
    EXPECT_THAT(result, ElementsAre( 0x7f, 0x7f ));
}

TEST(TestSplit, TestSplitThree)
{
    auto result = splitInto7Bits(0xffff);
    EXPECT_THAT(result, ElementsAre( 0x03, 0x7f, 0x7f ));
}

TEST(TestSplit, TestSplitThreeMax)
{
    auto result = splitInto7Bits(0x1fffff);
    EXPECT_THAT(result, ElementsAre( 0x7f, 0x7f, 0x7f ));
}


TEST(TestBuffer, TestOne) //NOLINT
{
    auto bytes = splitInto7Bits(1);
    EXPECT_THAT(bytes, ElementsAre(0x01));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    auto bufVector = std::vector<unsigned char>(buffer.get(), buffer.get() + bytes.size());
    EXPECT_THAT(bufVector, ElementsAre(0x01));
}

TEST(TestBuffer, TestOneByteMax) //NOLINT
{
    auto bytes = splitInto7Bits(0x7f);
    EXPECT_THAT(bytes, ElementsAre(0x7f));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    auto bufVector = std::vector<unsigned char>(buffer.get(), buffer.get() + bytes.size());
    EXPECT_THAT(bufVector, ElementsAre(0x7f));
}

TEST(TestBuffer, TestTwoBytes)
{
    auto bytes = splitInto7Bits(0xff);
    EXPECT_THAT(bytes, ElementsAre(0x01, 0x7f));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    auto bufVector = std::vector<unsigned char>(buffer.get(), buffer.get() + bytes.size());
    EXPECT_THAT(bufVector, ElementsAre(0x81, 0x7f));
}

TEST(TestBuffer, TestThreeBytes)
{
    auto bytes = splitInto7Bits(0xffff);
    EXPECT_THAT(bytes, ElementsAre(0x03, 0x7f, 0x7f));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    auto bufVector = std::vector<unsigned char>(buffer.get(), buffer.get() + bytes.size());
    EXPECT_THAT(bufVector, ElementsAre(0x83, 0xff, 0x7f));
}

TEST(TestBuffer, TestThreeBytesWithZeroes)
{
    auto bytes = splitInto7Bits(0x10000);
    EXPECT_THAT(bytes, ElementsAre(0x04, 0x00, 0x00));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    auto bufVector = std::vector<unsigned char>(buffer.get(), buffer.get() + bytes.size());
    EXPECT_THAT(bufVector, ElementsAre(0x84, 0x80, 0x00));
}

namespace
{
    class TestFdTransfer : public UnixSocketMemoryAppenderUsingTests
    {};

    class TestReadLength : public UnixSocketMemoryAppenderUsingTests
    {};

    class TestFile
    {
    public:
        [[maybe_unused]] explicit TestFile(std::string name)
            : m_name(std::move(name))
        {
            ::unlink(m_name.c_str());
        }

        TestFile()
        {
            const TestInfo* const test_info = UnitTest::GetInstance()->current_test_info();
            std::stringstream name;
            name << test_info->test_case_name() << "_" << test_info->name();
            m_name = name.str();
            ::unlink(m_name.c_str());
        }

        ~TestFile()
        {
            ::unlink(m_name.c_str());
        }

        void create() const
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_WRONLY | O_CREAT, 0666));
            ASSERT_GE(fd.get(), 0);
        }

        void write(const void* buffer, size_t length) const
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_WRONLY | O_CREAT, 0666));
            ASSERT_GE(fd.get(), 0);
            ssize_t ret = ::write(fd.get(), buffer, length);
            ASSERT_EQ(ret, length);
        }

        void write(const std::string& buffer) const
        {
            write(buffer.data(), buffer.size());
        }

        [[nodiscard]] int readFD() const
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_RDONLY));
            EXPECT_GE(fd.get(), 0);
            return fd.release();
        }

        std::string m_name;
    };

    class TestSocket
    {
    public:
        TestSocket()
        : socket_fds {-1}
        {
            ::socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
        }

        ~TestSocket()
        {
            ::close(socket_fds[0]);
            ::close(socket_fds[1]);
        }

        int get_client_fd()
        {
            return socket_fds[0];
        }

        int get_server_fd()
        {
            return socket_fds[1];
        }
    private:
        int socket_fds[2];
    };
}

TEST_F(TestReadLength, EOFReturnsMinus2)
{
    TestFile tf;
    tf.create();

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0) << "Open read-only failed: errno=" << errno << ": " <<
        common::safer_strerror(errno);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, -2);
}

TEST_F(TestReadLength, FailedRead)
{
    const std::string expected = "Reading socket returned error: ";
    UsingMemoryAppender memoryAppenderHolder(*this);

    TestFile tf;
    tf.create();

    datatypes::AutoFd fd(tf.readFD());

    ::close(fd.get());

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, -1);

    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestReadLength, TooLargeLengthReturnsMinusOne)
{
    TestFile tf;
    unsigned char buffer[] = { 0xff, 0xff, 0xff, 0xff, 0xff };
    tf.write(buffer, sizeof(buffer));

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, -1);
}
TEST_F(TestReadLength, ZeroLength)
{
    TestFile tf;
    unsigned char buffer[] = { 0x00 };
    tf.write(buffer, sizeof(buffer));

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, 0);
}

TEST_F(TestReadLength, TwoByteLength)
{
    TestFile tf;
    unsigned char buffer[] = { 0x81, 0x7f };
    tf.write(buffer, sizeof(buffer));

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, 0xff);
}

TEST_F(TestReadLength, MaxLength)
{
    TestFile tf;

    // maximum is ( ( 128 * 1024 ) << 7 ) + 127 )
    unsigned char buffer[] = { 0x88, 0x80, 0x80, 0x7f };
    tf.write(buffer, sizeof(buffer));

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, 0x0100007f);
}

TEST_F(TestReadLength, OverMaxLength)
{
    TestFile tf;

    // maximum is ( ( 128 * 1024 ) << 7 ) + 127 )
    unsigned char buffer[] = { 0x88, 0x80, 0x81, 0x00 };
    tf.write(buffer, sizeof(buffer));

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, -1);
}

TEST_F(TestReadLength, PaddedLength)
{
    TestFile tf;

    unsigned char buffer[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01 };
    tf.write(buffer, sizeof(buffer));

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    auto ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, 1);
}

TEST(TestWriteLength, TwoByteLength)
{
    TestSocket socket_pair;

    unixsocket::writeLength(socket_pair.get_client_fd(), 0xFF);

    char buffer[8] = { '\0' };
    ssize_t readlen = ::read(socket_pair.get_server_fd(), buffer, sizeof(buffer));

    ASSERT_EQ(readlen, 2);

    auto bufVector = std::vector<unsigned char>(buffer, buffer + readlen);
    EXPECT_THAT(bufVector, ElementsAre(0x81, 0x7f));
}

TEST(TestWriteLength, ZeroLength)
{
    TestSocket socket_pair;

    try
    {
        unixsocket::writeLength(socket_pair.get_client_fd(), 0);
        FAIL() << "writeLength() didn't throw";
    }
    catch (std::runtime_error &e)
    {
        EXPECT_STREQ(e.what(), "Attempting to write length of zero");
    }
}

TEST(TestWriteLength, WriteError)
{
    TestSocket socket_pair;

    ::close(socket_pair.get_client_fd());

    try
    {
        unixsocket::writeLength(socket_pair.get_client_fd(), 1);
        FAIL() << "writeLength() didn't throw";
    }
    catch (environmentInterruption &e)
    {
        ASSERT_STREQ(e.what(), "Environment interruption");
    }
}

TEST(TestWriteLengthAndBuffer, SimpleWrite)
{
    std::string client_buffer = "hello";
    TestSocket socket_pair;

    unixsocket::writeLengthAndBuffer(socket_pair.get_client_fd(), client_buffer);

    char server_buffer[16] = { '\0' };
    ssize_t readlen = ::read(socket_pair.get_server_fd(), server_buffer, sizeof(server_buffer));
    ASSERT_EQ(readlen, 6);
    ASSERT_EQ(server_buffer[0], client_buffer.length());
    EXPECT_STREQ(server_buffer+1, client_buffer.c_str());
}

TEST_F(TestFdTransfer, validFd)
{
    TestSocket socket_pair;

    TestFile tf;
    tf.write("foo");

    datatypes::AutoFd client_fd(tf.readFD());

    auto ret = unixsocket::send_fd(socket_pair.get_client_fd(), client_fd.get());
    ASSERT_GT(ret, 0) << "send_fd failed with: " << common::safer_strerror(errno);

    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_GE(new_fd, 0) << "recv_fd failed with: " << common::safer_strerror(errno);
    datatypes::AutoFd server_fd(new_fd);

    ASSERT_NE(server_fd.get(), client_fd.get());

    char buffer[10] {};
    ssize_t len = ::read(server_fd.get(), buffer, sizeof(buffer) - 1);
    ASSERT_GT(len, 0);
    ASSERT_STREQ(buffer, "foo");
}

TEST_F(TestFdTransfer, writeError)
{
    TestSocket socket_pair;

    TestFile tf;
    tf.write("foo");

    datatypes::AutoFd client_fd(tf.readFD());

    ::close(socket_pair.get_client_fd());
    auto ret = unixsocket::send_fd(socket_pair.get_client_fd(), client_fd.get());
    ASSERT_EQ(ret, -1);
}

TEST_F(TestFdTransfer, readError)
{
    TestSocket socket_pair;

    ::close(socket_pair.get_server_fd());
    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_EQ(new_fd, -1);
}

static void send_fds(int socket, int* fds, int count)  // send fd by socket
{
    std::vector<char> buf(CMSG_SPACE(sizeof(int) * count ));
    int dup = 0;
    struct iovec io = { .iov_base = &dup, .iov_len = sizeof(dup) };

    struct msghdr msg = {};
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf.data();
    msg.msg_controllen = buf.size();

    struct cmsghdr *cmsg;
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int) * count);

    memcpy (CMSG_DATA(cmsg), fds, sizeof(int) * count);

    ssize_t ret = ::sendmsg(socket, &msg, 0);
    ASSERT_GE(ret, 0);
}

static int count_open_fds()
{
    int count = 0;
    for (int fd=0; fd<=255; ++fd)
    {
        int dup_fd = dup(fd);
        if (dup_fd >= 0)
        {
            ++count;
            close(dup_fd);
        }
    }
    return count;
}

TEST_F(TestFdTransfer, sendTwoFds)
{
    const std::string expected = "Control data was truncated when receiving fd";
    UsingMemoryAppender memoryAppenderHolder(*this);
    TestSocket socket_pair;

    int fd_count_before = count_open_fds();

    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    datatypes::AutoFd devZero(::open("/dev/zero", O_RDONLY));
    int fds[] = { devNull.get(), devZero.get() };
    send_fds(socket_pair.get_client_fd(), fds, 2); // send two file descriptors
    devZero.close();
    devNull.close();

    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_EQ(new_fd, -1);

    int fd_count_after = count_open_fds();
    EXPECT_EQ(fd_count_before, fd_count_after);
    EXPECT_FALSE(appenderContains(expected));
}

TEST_F(TestFdTransfer, sendThreeFds)
{
    const std::string expected = "Control data was truncated when receiving fd";

    UsingMemoryAppender memoryAppenderHolder(*this);
    TestSocket socket_pair;

    int fd_count_before = count_open_fds();

    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    datatypes::AutoFd devZero(::open("/dev/zero", O_RDONLY));
    datatypes::AutoFd devFull(::open("/dev/full", O_RDONLY));
    int fds[] = { devNull.get(), devZero.get(), devFull.get() };
    send_fds(socket_pair.get_client_fd(), fds, 3); // send three file descriptors
    devFull.close();
    devZero.close();
    devNull.close();

    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_EQ(new_fd, -1);

    int fd_count_after = count_open_fds();
    EXPECT_EQ(fd_count_before, fd_count_after);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestFdTransfer, sendZeroFds)
{
    const std::string expected = "Failed to receive fd: ";
    UsingMemoryAppender memoryAppenderHolder(*this);
    TestSocket socket_pair;

    int fds[] = {};
    send_fds(socket_pair.get_client_fd(), fds, 0);

    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_EQ(new_fd, -1);

    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestFdTransfer, sendRegularMessage)
{
    const std::string expected = "Failed to receive fd: ";
    UsingMemoryAppender memoryAppenderHolder(*this);
    TestSocket socket_pair;

    std::string buffer = "junk";
    ::send(socket_pair.get_client_fd(), buffer.c_str(), buffer.size(), MSG_NOSIGNAL);

    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_EQ(new_fd, -1);

    EXPECT_TRUE(appenderContains(expected));
}

TEST(TestSocketUtils, environmentInterruptionReportsWhat)
{
    try
    {
        throw unixsocket::environmentInterruption();
    }
    catch (const unixsocket::environmentInterruption& ex)
    {
        EXPECT_EQ(ex.what(), "Environment interruption");
    }
}
