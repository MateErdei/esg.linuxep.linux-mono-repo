// Copyright 2023 Sophos All rights reserved.

#include "unixsocket/ReadLengthAsync.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "Common/Helpers/MockSysCalls.h"

namespace
{
    class TestReadLengthAsync : public UnixSocketMemoryAppenderUsingTests
    {};
}

TEST_F(TestReadLengthAsync, construction)
{
    auto mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();
    unixsocket::ReadLengthAsync reader(mockSysCalls, 1000);
}
