// Copyright 2023 Sophos All rights reserved.

#include "unixsocket/ReadLengthAsync.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "Common/Helpers/MockSysCalls.h"

namespace
{
    class TestReadLengthAsync : public UnixSocketMemoryAppenderUsingTests
    {
    public:
    };
}

TEST_F(TestReadLengthAsync, construction)
{
    auto mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 1000);
}

TEST_F(TestReadLengthAsync, readZero)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 1000);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<char*>(outputbuf)[0] = 0; }),
            Return(1)
                ));

    auto ret = reader.read(42);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(reader.getLength(), 0);
}

TEST_F(TestReadLengthAsync, readSmall)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 1000);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<char*>(outputbuf)[0] = 127; }),
            Return(1)
                ));

    auto ret = reader.read(42);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(reader.getLength(), 127);
}

TEST_F(TestReadLengthAsync, faultInjection_zero_continuation)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 1000);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillRepeatedly(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x80; }),
            Return(1)
                ));

    auto ret = reader.read(42);
    auto error = errno;
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(error, EINVAL);
}

TEST_F(TestReadLengthAsync, readLarge)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 1000);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x81; }),
            Return(1)
                ))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x1; }),
            Return(1)
                ));

    auto ret = reader.read(42);
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(reader.getLength(), 129);
}

TEST_F(TestReadLengthAsync, readLargeWithZeroContinuation)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 100000);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x81; }),
            Return(1)
                ))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x80; }),
            Return(1)
                ))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x01; }),
            Return(1)
                ));

    auto ret = reader.read(42);
    EXPECT_EQ(ret, 3);
    EXPECT_EQ(reader.getLength(), 16385);
}

TEST_F(TestReadLengthAsync, enforceSmallLimit)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 100);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<char*>(outputbuf)[0] = 127; }),
            Return(1)
                ));

    errno = 0;
    auto ret = reader.read(42);
    auto error = errno;
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(error, E2BIG);
    EXPECT_FALSE(reader.complete());
}

TEST_F(TestReadLengthAsync, enforceLargeLimit)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 100000);

    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 1))
        .WillRepeatedly(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { reinterpret_cast<unsigned char*>(outputbuf)[0] = 0x81; }),
            Return(1)
                ));

    errno = 0;
    auto ret = reader.read(42);
    auto error = errno;
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(error, E2BIG);
    EXPECT_FALSE(reader.complete());
}
