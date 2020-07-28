/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/SocketUtils.h"
#include "unixsocket/SocketUtilsImpl.h"

#include <datatypes/AutoFd.h>
#include <datatypes/Print.h>
#include <gtest/gtest.h>

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
