/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "tests/common/LogInitializedTests.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerConnectionThread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestThreatReporterServerConectionThread : public LogInitializedTests
    {};


    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(const std::string& threatDetectedXML));
    };
}

TEST_F(TestThreatReporterServerConectionThread, successful_construction) //NOLINT
{
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);

    EXPECT_NO_THROW(unixsocket::ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback));
}

TEST_F(TestThreatReporterServerConectionThread, isRunning_false_after_construction) //NOLINT
{
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);

    unixsocket::ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback);
    EXPECT_FALSE(connectionThread.isRunning());
}




