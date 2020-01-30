/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "unixsocket/SocketUtils.h"
#include "unixsocket/SocketUtilsImpl.h"

#include <deque>

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
