/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Common/ProcessMonitoring/IProcessMonitor.h>
#include <Common/ProcessMonitoringImpl/ProcessMonitor.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <future>
#include <thread>
#include <signal.h>


namespace
{
    void* processMonitorExecuter(void* args)
    {
        auto processMonitor = static_cast<Common::ProcessMonitoring::IProcessMonitor*>(args);
        int* result = new int(processMonitor->run());
        return static_cast<void*>(result);
    }

    class ProcessMonitorThread
    {
    public:
        ProcessMonitorThread(Common::ProcessMonitoring::IProcessMonitorPtr processMonitorPtr)
                : m_processMonitorPtr(std::move(processMonitorPtr))
        {}

        void start()
        {
            int s = pthread_create(&m_thread_id, NULL, processMonitorExecuter, m_processMonitorPtr.get());
            ASSERT_EQ(0, s);
        }

        int stop()
        {
            pthread_kill(m_thread_id, SIGTERM);
            void* res;
            pthread_join(m_thread_id, &res);
            int result = (*static_cast<int*>(res));
            delete static_cast<int*>(res);
            return result;
        }

    private:
        Common::ProcessMonitoring::IProcessMonitorPtr m_processMonitorPtr;
        pthread_t m_thread_id;
    };
}


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

TEST_F(ProcessMonitoring, createProcessMonitorStartWithProcessDoesNotRaiseErrorAndProcessMonitorStopsOnSIGTERMSignal)  //NOLINT
{
    testing::internal::CaptureStderr();
    auto processMonitorPtr = Common::ProcessMonitoring::createProcessMonitor();
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    auto proxyPtr = Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr));
    processMonitorPtr->addProcessToMonitor(std::move(proxyPtr));
    ProcessMonitorThread  processMonitorThread(std::move(processMonitorPtr));

    processMonitorThread.start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int result = processMonitorThread.stop();

    EXPECT_EQ(result, 0);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("No processes to monitor!")));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Not starting plugin without executable"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Process Monitoring Exiting"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Stopping processes"));
}
