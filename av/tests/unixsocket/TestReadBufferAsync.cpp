// Copyright 2023 Sophos All rights reserved.

#include "unixsocket/ReadBufferAsync.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "Common/Helpers/MockSysCalls.h"

namespace
{
    class TestReadBufferAsync : public UnixSocketMemoryAppenderUsingTests
    {};
}

TEST_F(TestReadBufferAsync, construct)
{
    auto mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();
    unixsocket::ReadBufferAsync buffer{mockSysCalls, 512};
    EXPECT_TRUE(buffer.complete());
}

TEST_F(TestReadBufferAsync, setLength)
{
    auto mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();
    unixsocket::ReadBufferAsync buffer{mockSysCalls, 512};
    buffer.setLength(10);
    EXPECT_FALSE(buffer.complete());
}


TEST_F(TestReadBufferAsync, pipe)
{
    int pipefd[2];
    ssize_t ret = ::pipe(pipefd);
    ASSERT_NE(ret, -1);

    const auto* EXPECTED = "123456789";
    ret = ::write(pipefd[1], EXPECTED, 10);
    ASSERT_EQ(ret, 10);

    char output[10];
    ret = ::read(pipefd[0], output, 10);
    EXPECT_EQ(ret, 10);

    EXPECT_STREQ(output, EXPECTED);
}

TEST_F(TestReadBufferAsync, readCompleteMock)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadBufferAsync buffer{mockSysCalls, 512};

    buffer.setLength(10);
    ASSERT_FALSE(buffer.complete());

    static const auto* EXPECTED = "123456789";
    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 10))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { std::memcpy(outputbuf, EXPECTED, 10 ); }),
            Return(10)
            ));

    auto ret = buffer.read(42);
    EXPECT_EQ(ret, 10);

    auto& buf = buffer.getBuffer();
    char* charbuf = reinterpret_cast<char*>(buf.begin());
    EXPECT_STREQ(charbuf, EXPECTED);
}

TEST_F(TestReadBufferAsync, readCompleteReal)
{
    auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
    unixsocket::ReadBufferAsync buffer{sysCalls, 512};

    buffer.setLength(10);
    ASSERT_FALSE(buffer.complete());

    int pipefd[2];
    ssize_t ret = ::pipe(pipefd);
    ASSERT_NE(ret, -1);

    const auto* EXPECTED = "123456789";
    ret = ::write(pipefd[1], EXPECTED, 10);
    ASSERT_EQ(ret, 10);

    ret = buffer.read(pipefd[0]);
    EXPECT_EQ(ret, 10);

    auto& buf = buffer.getBuffer();
    char* charbuf = reinterpret_cast<char*>(buf.begin());
    EXPECT_STREQ(charbuf, EXPECTED);
}


TEST_F(TestReadBufferAsync, readPartial)
{
    auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    unixsocket::ReadBufferAsync buffer{mockSysCalls, 512};

    buffer.setLength(10);
    ASSERT_FALSE(buffer.complete());

    static const auto* EXPECTED = "123456789";
    void* outputbuf;
    EXPECT_CALL(*mockSysCalls, read(42, _, 10))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { std::memcpy(outputbuf, EXPECTED, 1 ); }),
            Return(1)
                ));
    EXPECT_CALL(*mockSysCalls, read(42, _, 9))
        .WillOnce(DoAll(
            SaveArg<1>(&outputbuf),
            Invoke([&outputbuf]() { std::memcpy(outputbuf, EXPECTED+1, 9 ); }),
            Return(9)
                ));

    auto ret = buffer.read(42);
    EXPECT_EQ(ret, 1);
    EXPECT_FALSE(buffer.complete());

    ret = buffer.read(42);
    EXPECT_EQ(ret, 9);
    EXPECT_TRUE(buffer.complete());

    auto& buf = buffer.getBuffer();
    char* charbuf = reinterpret_cast<char*>(buf.begin());
    EXPECT_STREQ(charbuf, EXPECTED);
}
