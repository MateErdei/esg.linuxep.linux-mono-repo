/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Common/ProcessMonitoring/IProcessMonitor.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ProcessMonitoring : public ::testing::Test
{
public:
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
};

TEST_F(ProcessMonitoring, createProcessMonitorDoesntThrow)  //NOLINT
{
    ASSERT_NO_THROW(Common::ProcessMonitoring::createProcessMonitor());
}

TEST_F(ProcessMonitoring, createProcessMonitorStartWithOutProcessRaisesError)  //NOLINT
{
    testing::internal::CaptureStderr();
    auto processMonitor = Common::ProcessMonitoring::createProcessMonitor();
    int retCode = processMonitor->run();
    EXPECT_EQ(retCode, 1);
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("No processes to monitor!"));
}