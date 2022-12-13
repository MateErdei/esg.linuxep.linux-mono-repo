// Copyright 2022 Sophos Limited. All rights reserved.

#include "common/MemoryAppender.h"
#include "datatypes/MockSysCalls.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ProcessForceExitTimer.h"

#include <gtest/gtest.h>

using namespace sspl::sophosthreatdetectorimpl;
using namespace testing;

namespace
{
    class TestProcessForceExitTimer : public MemoryAppenderUsingTests
    {
    public:
        TestProcessForceExitTimer() :  MemoryAppenderUsingTests("SophosThreatDetectorImpl")
        {}

        void SetUp() override
        {
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
    };
}

TEST_F(TestProcessForceExitTimer, testTimeout)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int expectedExitCode = common::E_SCAN_ABORTED_WITH_THREATS;
    EXPECT_CALL(*m_mockSysCalls, _exit(expectedExitCode));

    using namespace std::chrono_literals;
    auto processForceExitTimer = std::make_shared<ProcessForceExitTimer>(0s, m_mockSysCalls);
    processForceExitTimer->setExitCode(expectedExitCode);
    processForceExitTimer->start();

    std::stringstream expectedLogMsg;
    expectedLogMsg << "Timed out waiting for graceful shutdown - forcing exit with return code " << expectedExitCode;
    ASSERT_TRUE(waitForLog(expectedLogMsg.str()));
}

TEST_F(TestProcessForceExitTimer, testStop)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int expectedExitCode = common::E_SCAN_ABORTED_WITH_THREATS;

    using namespace std::chrono_literals;
    auto processForceExitTimer = std::make_shared<ProcessForceExitTimer>(2s, m_mockSysCalls);
    processForceExitTimer->setExitCode(expectedExitCode);
    processForceExitTimer->start();
    std::this_thread::sleep_for(500ms);
    processForceExitTimer->tryStop();
    processForceExitTimer->join();

    std::stringstream expectedLogMsg;
    expectedLogMsg << "Timed out waiting for graceful shutdown - forcing exit with return code " << expectedExitCode;
    EXPECT_FALSE(appenderContains(expectedLogMsg.str()));
}