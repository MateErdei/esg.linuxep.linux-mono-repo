// Copyright 2023 Sophos All rights reserved.

#include "unixsocket/SocketUtils.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "Common/Helpers/MockSysCalls.h"

namespace
{
    class TestReadFully : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        std::shared_ptr<MockSystemCallWrapper> m_mockSysCalls;

    };
}

TEST_F(TestReadFully, read_once)
{
    int fd = 42;
    char* buffer = nullptr;
    ssize_t length = 1;
    EXPECT_CALL(*m_mockSysCalls, read(fd, buffer, length)).WillOnce(Return(1));
    auto result = unixsocket::readFully(m_mockSysCalls,
            fd, buffer, length, std::chrono::milliseconds{1});
    EXPECT_EQ(result, 1);
}

TEST_F(TestReadFully, read_too_much)
{
    int fd = 42;
    char* buffer = nullptr;
    ssize_t length = 1;
    EXPECT_CALL(*m_mockSysCalls, read(fd, buffer, length)).WillOnce(Return(2));
    auto result = unixsocket::readFully(m_mockSysCalls,
                                        fd, buffer, length, std::chrono::milliseconds{1});
    EXPECT_EQ(result, 2);
}

TEST_F(TestReadFully, read_too_little)
{
    int fd = 42;
    char* buffer = nullptr;
    ssize_t length = 2;
    EXPECT_CALL(*m_mockSysCalls, read(fd, buffer, length)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockSysCalls, read(fd, buffer+1, length-1)).WillOnce(Return(1));
    auto result = unixsocket::readFully(m_mockSysCalls,
                                        fd, buffer, length, std::chrono::milliseconds{1});
    EXPECT_EQ(result, 2);
}
