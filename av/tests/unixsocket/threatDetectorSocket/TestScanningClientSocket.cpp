// Copyright 2022 Sophos Limited. All rights reserved.


#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "../UnixSocketMemoryAppenderUsingTests.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class TestScanningClientSocket : public UnixSocketMemoryAppenderUsingTests
    {
    };
}

TEST_F(TestScanningClientSocket, construction)
{
    EXPECT_NO_THROW(unixsocket::ScanningClientSocket socket("/tmp/socket"));
}
