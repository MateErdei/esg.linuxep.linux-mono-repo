/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/SocketUtils.h"
#include "unixsocket/SocketUtilsImpl.h"

#include "UnixSocketMemoryAppenderUsingTests.h"
#include <datatypes/AutoFd.h>
#include <datatypes/Print.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <deque>
#include <fcntl.h>


using namespace unixsocket;

TEST(TestSplit, TestSplitOneByte) // NOLINT
{
    auto result = splitInto7Bits(100);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.at(0), static_cast<uint8_t>(100));
}

TEST(TestSplit, TestSplitTwoBytes) // NOLINT
{
    auto result = splitInto7Bits(0xff);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result.at(0), static_cast<uint8_t>(1));
    EXPECT_EQ(result.at(1), static_cast<uint8_t>(0x7f));
}

TEST(TestSplit, TestSplitTwoBytesMax) // NOLINT
{

    auto result = splitInto7Bits(0x3fff);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result.at(0), static_cast<uint8_t>(0x7f));
    EXPECT_EQ(result.at(1), static_cast<uint8_t>(0x7f));
}

TEST(TestSplit, TestSplitThree) // NOLINT
{
    auto result = splitInto7Bits(0xffff);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result.at(0), static_cast<uint8_t>(3));
    EXPECT_EQ(result.at(1), static_cast<uint8_t>(0x7f));
    EXPECT_EQ(result.at(2), static_cast<uint8_t>(0x7f));
}

TEST(TestSplit, TestSplitThreeMax) // NOLINT
{
    auto result = splitInto7Bits(0x1fffff);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result.at(0), static_cast<uint8_t>(0x7f));
    EXPECT_EQ(result.at(1), static_cast<uint8_t>(0x7f));
    EXPECT_EQ(result.at(2), static_cast<uint8_t>(0x7f));
}


TEST(TestBuffer, TestOne) //NOLINT
{
    auto bytes = splitInto7Bits(1);
    ASSERT_EQ(bytes.size(), 1);
    EXPECT_EQ(bytes.at(0), static_cast<uint8_t>(1));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    EXPECT_EQ(buffer[0], static_cast<uint8_t>(1));
}

TEST(TestBuffer, TestOneByteMax) //NOLINT
{
    auto bytes = splitInto7Bits(0x7f);
    ASSERT_EQ(bytes.size(), 1);
    EXPECT_EQ(bytes.at(0), static_cast<uint8_t>(0x7f));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    EXPECT_EQ(buffer[0], static_cast<uint8_t>(0x7f));
}

TEST(TestBuffer, TestSplitTwoBytes) // NOLINT
{
    auto bytes = splitInto7Bits(0xff);
    ASSERT_EQ(bytes.size(), 2);
    EXPECT_EQ(bytes.at(0), static_cast<uint8_t>(1));
    EXPECT_EQ(bytes.at(1), static_cast<uint8_t>(0x7f));

    auto buffer = addTopBitAndPutInBuffer(bytes);
    EXPECT_EQ(buffer[0], static_cast<uint8_t>(0x81));
    EXPECT_EQ(buffer[1], static_cast<uint8_t>(0x7f));
}

namespace
{
    class TestFdTransfer : public UnixSocketMemoryAppenderUsingTests
    {};

    class TestFile
    {
    public:
        TestFile(const char* name)
            : m_name(name)
        {
            ::unlink(m_name.c_str());
        }

        ~TestFile()
        {
            ::unlink(m_name.c_str());
        }

        void create()
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_WRONLY | O_CREAT, 0666));
        }

        void write(const char* buffer, size_t length)
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_WRONLY | O_CREAT, 0666));
            ASSERT_GE(fd.get(), 0);
            int ret = ::write(fd.get(), buffer, length);
            ASSERT_EQ(ret, length);
        }

        void write(const std::string& buffer)
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_WRONLY | O_CREAT, 0666));
            ASSERT_GE(fd.get(), 0);
            int ret = ::write(fd.get(), buffer.data(), buffer.size());
            ASSERT_EQ(ret, buffer.size());
        }

        int readFD()
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), O_RDONLY));
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

TEST(TestReadLength, EOFReturnsMinus2) // NOLINT
{
    TestFile tf("TestReadLength_EOFReturnsMinus2_buffer");
    tf.create();

    datatypes::AutoFd fd(tf.readFD());
    if (fd.get() < 0)
    {
        PRINT("Open read-only failed: errno=" << errno << ": " << strerror(errno));
        ASSERT_GE(fd.get(), 0);
    }

    int ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, -2);
}

TEST(TestReadLength, TooLargeLengthReturnsMinusOne) // NOLINT
{
    TestFile tf("TestReadLength_TooLargeLengthReturnsMinusOne_buffer");
    tf.write("\xFF\xFF\xFF\xFF\xFF");

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    int ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, -1);
}

TEST(TestReadLength, TwoByteLength) // NOLINT
{
    TestFile tf("TestReadLength_TwoByteLength");
    tf.write("\x81\x7F");

    datatypes::AutoFd fd(tf.readFD());
    ASSERT_GE(fd.get(), 0);

    int ret = unixsocket::readLength(fd.get());
    EXPECT_EQ(ret, 0xFF);
}

TEST_F(TestFdTransfer, validFd) // NOLINT
{
    TestSocket socket_pair;

    TestFile tf("TestSocketUtils_passFd");
    tf.write("foo");

    datatypes::AutoFd client_fd(tf.readFD());

    unixsocket::send_fd(socket_pair.get_client_fd(), client_fd.get());

    int new_fd = unixsocket::recv_fd(socket_pair.get_server_fd());
    ASSERT_GE(new_fd, 0);
    datatypes::AutoFd server_fd(new_fd);

    ASSERT_NE(server_fd.get(), client_fd.get());

    char buffer[10] {};
    int ret = ::read(server_fd.get(), buffer, sizeof(buffer) - 1);
    ASSERT_GT(ret, 0);
    ASSERT_STREQ(buffer, "foo");
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

    memcpy (CMSG_DATA(cmsg), fds, sizeof (int) * count);

    int ret = sendmsg (socket, &msg, 0);
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

TEST_F(TestFdTransfer, sendTwoFds) // NOLINT
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

TEST_F(TestFdTransfer, sendThreeFds) // NOLINT
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

TEST(TestSocketUtils, environmentInterruptionReportsWhat) // NOLINT
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
