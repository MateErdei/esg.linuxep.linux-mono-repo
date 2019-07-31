/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessMonitoring/IProcessMonitor.h>
#include <Common/ProcessMonitoringImpl/ProcessMonitor.h>
#include <Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>
#include <signal.h>
#include <thread>

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
        ProcessMonitorThread(Common::ProcessMonitoring::IProcessMonitorPtr processMonitorPtr) :
            m_processMonitorPtr(std::move(processMonitorPtr)),
            m_thread_id{}
        {
        }

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
} // namespace

class ProcessMonitoring : public ::testing::Test
{
public:
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
};

TEST_F(ProcessMonitoring, createProcessMonitorDoesntThrow) // NOLINT
{
    ASSERT_NO_THROW(Common::ProcessMonitoring::createProcessMonitor());
}

TEST_F(ProcessMonitoring, createProcessMonitorStartWithOutProcessRaisesError) // NOLINT
{
    testing::internal::CaptureStderr();
    auto processMonitor = Common::ProcessMonitoring::createProcessMonitor();
    int retCode = processMonitor->run();
    EXPECT_EQ(retCode, 1);
    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("No processes to monitor!"));
}

TEST_F(
    ProcessMonitoring,
    createProcessMonitorStartWithProcessDoesNotRaiseErrorAndProcessMonitorStopsOnSIGTERMSignal) // NOLINT
{
    testing::internal::CaptureStderr();
    auto processMonitorPtr = Common::ProcessMonitoring::createProcessMonitor();
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    auto proxyPtr = Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr));
    processMonitorPtr->addProcessToMonitor(std::move(proxyPtr));
    ProcessMonitorThread processMonitorThread(std::move(processMonitorPtr));

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

TEST_F(ProcessMonitoring, createProcessMonitorSocketHandlersThatThrowExceptionsThatAreCaught) // NOLINT
{
    testing::internal::CaptureStderr();

    auto context = Common::ZMQWrapperApi::createContext();
    auto processMonitorPtr =
        Common::ProcessMonitoring::IProcessMonitorPtr(new Common::ProcessMonitoringImpl::ProcessMonitor(context));
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    auto proxyPtr = Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr));
    processMonitorPtr->addProcessToMonitor(std::move(proxyPtr));

    auto requestorSocket1 = context->getRequester();
    auto replierSocket1 = context->getReplier();
    replierSocket1->listen("inproc://socket1");
    requestorSocket1->connect("inproc://socket1");
    requestorSocket1->setConnectionTimeout(100);
    requestorSocket1->setTimeout(100);

    auto requestorSocket2 = context->getRequester();
    auto replierSocket2 = context->getReplier();
    replierSocket2->listen("inproc://socket2");
    requestorSocket2->connect("inproc://socket2");
    requestorSocket2->setConnectionTimeout(100);
    requestorSocket2->setTimeout(100);

    auto blockingRequestorSocket = context->getRequester();
    auto blockingReplierSocket = context->getReplier();
    blockingReplierSocket->listen("inproc://blocker");
    blockingRequestorSocket->connect("inproc://blocker");
    blockingRequestorSocket->setConnectionTimeout(100);
    blockingRequestorSocket->setTimeout(100);

    auto replierSocket1BarePtr = replierSocket1.get();
    processMonitorPtr->addReplierSocketAndHandleToPoll(replierSocket1.get(), [replierSocket1BarePtr]() {
        replierSocket1BarePtr->read();
        replierSocket1BarePtr->write({ "response" });
        throw Common::ZeroMQWrapper::IIPCException("IPC error");
    });

    auto replierSocket2BarePtr = replierSocket2.get();
    processMonitorPtr->addReplierSocketAndHandleToPoll(replierSocket2.get(), [replierSocket2BarePtr]() {
        replierSocket2BarePtr->read();
        replierSocket2BarePtr->write({ "response" });
        throw int(1);
    });

    auto blockingReplierSocketBarePtr = blockingReplierSocket.get();
    processMonitorPtr->addReplierSocketAndHandleToPoll(blockingReplierSocket.get(), [blockingReplierSocketBarePtr]() {
        blockingReplierSocketBarePtr->read();
        blockingReplierSocketBarePtr->write({ "response" });
    });

    ProcessMonitorThread processMonitorThread(std::move(processMonitorPtr));
    processMonitorThread.start();

    requestorSocket1->write({ "stuff1" });
    requestorSocket1->read();
    blockingRequestorSocket->write({ "checksent" });
    blockingRequestorSocket->read();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("No processes to monitor!")));
    EXPECT_THAT(
        logMessage, ::testing::HasSubstr("Unexpected error occurred when handling socket communication: IPC error"));

    testing::internal::CaptureStderr();

    requestorSocket2->write({ "stuff2" });
    requestorSocket2->read();
    blockingRequestorSocket->write({ "checksent" });
    blockingRequestorSocket->read();

    int result = processMonitorThread.stop();
    EXPECT_EQ(result, 0);

    logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(
        logMessage,
        ::testing::Not(
            ::testing::HasSubstr("Unexpected error occurred when handling socket communication: IPC error")));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unknown error occurred when handling socket communication."));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Process Monitoring Exiting"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Stopping processes"));
}

TEST_F(ProcessMonitoring, createProcessMonitorAndCheckSIGCHLDSignalIsHandled) // NOLINT
{
    testing::internal::CaptureStderr();

    auto context = Common::ZMQWrapperApi::createContext();
    auto processMonitorPtr =
        Common::ProcessMonitoring::IProcessMonitorPtr(new Common::ProcessMonitoringImpl::ProcessMonitor(context));
    auto processInfoPtr = Common::Process::createEmptyProcessInfo();
    auto proxyPtr = Common::ProcessMonitoring::createProcessProxy(std::move(processInfoPtr));
    processMonitorPtr->addProcessToMonitor(std::move(proxyPtr));

    auto requestorSocket1 = context->getRequester();
    auto replierSocket1 = context->getReplier();
    replierSocket1->listen("inproc://socket1");
    requestorSocket1->connect("inproc://socket1");
    requestorSocket1->setConnectionTimeout(100);
    requestorSocket1->setTimeout(100);

    auto replierSocket1BarePtr = replierSocket1.get();
    processMonitorPtr->addReplierSocketAndHandleToPoll(replierSocket1.get(), [replierSocket1BarePtr]() {
        replierSocket1BarePtr->read();
        replierSocket1BarePtr->write({ "response" });
        raise(SIGCHLD);
    });

    ProcessMonitorThread processMonitorThread(std::move(processMonitorPtr));
    processMonitorThread.start();

    requestorSocket1->write({ "stuff1" });
    auto reply = requestorSocket1->read();

    int result = processMonitorThread.stop();
    EXPECT_EQ(result, 0);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Process Monitoring Exiting"));
    EXPECT_THAT(logMessage, ::testing::Not(::testing::HasSubstr("ERROR")));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Stopping processes"));
}
