/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "unixsocket/SocketUtils.h"

#include <deque>

static std::deque<uint8_t> splitInto7Bits(unsigned length)
{
    std::deque<uint8_t> bytes;
    while (length > 0)
    {
        uint8_t c = length & static_cast<uint8_t>(127);
        bytes.push_front(c);
        length >>= static_cast<uint8_t>(7);
    }
    return bytes;
}

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


static std::unique_ptr<uint8_t[]> addTopBitAndPutInBuffer(const std::deque<uint8_t>& bytes)
{
    const uint8_t TOP_BIT = 0x80;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bytes.size()]);
    for(size_t i=0; i < bytes.size()-1; i++)
    {
        buffer[i] = bytes[i] | TOP_BIT;
    }
    buffer[bytes.size()-1] = bytes[bytes.size()-1];
    return buffer;
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
