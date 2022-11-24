// Copyright 2022 Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "../common/Common.h"
#include "../common/WaitForEvent.h"
#include "common/NotifyPipeSleeper.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingClient.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingServer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestRestoreReportSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override { setupFakePluginConfig(); }

        void TearDown() override { sophos_filesystem::remove_all(tmpdir()); }
    };

    class MockRestoreReportProcessor : public IRestoreReportProcessor
    {
    public:
        MOCK_METHOD(void, processRestoreReport, (const scan_messages::RestoreReport& restoreReport), (const)); // NOLINT
    };
} // namespace

TEST_F(TestRestoreReportSocket, TestSendRestoreReport)
{
    const scan_messages::RestoreReport restoreReport{
        0, "/path/eicar.txt", "00000000-0000-0000-0000-000000000000", true
    };

    MockRestoreReportProcessor mockReporter;
    WaitForEvent guard;
    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport))
        .WillOnce(InvokeWithoutArgs([&guard]() { guard.onEventNoArgs(); }));

    UsingMemoryAppender memoryAppenderHolder(*this);

    {
        RestoreReportingServer server{ mockReporter };

        server.start();

        {
            RestoreReportingClient client{ nullptr };
            client.sendRestoreReport(restoreReport);
        }

        guard.wait();

        EXPECT_TRUE(appenderContains("Restore Reporting Server starting listening on socket: "));
        EXPECT_TRUE(appenderContains("Restore Reporting Server got connection with fd "));
        EXPECT_TRUE(appenderContains("Reporting successful restoration of /path/eicar.txt"));
        EXPECT_TRUE(appenderContains("Correlation ID: 00000000-0000-0000-0000-000000000000"));
        EXPECT_TRUE(appenderContains("Restore Reporting Server ready to receive data"));
        EXPECT_TRUE(appenderContains("Restore Reporting Server received a report"));
        EXPECT_TRUE(appenderContains(
            "Path: /path/eicar.txt, correlation ID: 00000000-0000-0000-0000-000000000000, successful"));

        // This means the connection has closed
        EXPECT_TRUE(waitForLog("Restore Reporting Server awaiting new connection", 100ms));
    }

    EXPECT_TRUE(appenderContains("Closing Restore Reporting Server socket"));
}

TEST_F(TestRestoreReportSocket, TestSendUnsuccessfulRestoreReport)
{
    const scan_messages::RestoreReport restoreReport{
        0, "/path/eicar.txt", "00000000-0000-0000-0000-000000000000", false
    };

    MockRestoreReportProcessor mockReporter;
    WaitForEvent guard;
    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport))
        .WillOnce(InvokeWithoutArgs([&guard]() { guard.onEventNoArgs(); }));

    internal::CaptureStderr();

    RestoreReportingServer server{ mockReporter };
    server.start();

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport);
    }

    guard.wait();

    const auto log = internal::GetCapturedStderr();
    EXPECT_THAT(log, HasSubstr(" INFO Reporting unsuccessful restoration of /path/eicar.txt"));
    EXPECT_THAT(
        log,
        HasSubstr("DEBUG Path: /path/eicar.txt, correlation ID: 00000000-0000-0000-0000-000000000000, unsuccessful"));
}

TEST_F(TestRestoreReportSocket, TestSendTwoReports)
{
    const scan_messages::RestoreReport restoreReport1{
        0, "/path/eicar1.txt", "00000000-0000-0000-0000-000000000000", true
    };

    const scan_messages::RestoreReport restoreReport2{
        0, "/path/eicar2.txt", "11111111-1111-1111-1111-111111111111", false
    };

    MockRestoreReportProcessor mockReporter;
    WaitForEvent guard1;
    WaitForEvent guard2;

    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport1))
        .WillOnce(InvokeWithoutArgs([&guard1]() { guard1.onEventNoArgs(); }));
    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport2))
        .WillOnce(InvokeWithoutArgs([&guard2]() { guard2.onEventNoArgs(); }));

    {
        {
            RestoreReportingServer server{ mockReporter };
            server.start();

            {
                internal::CaptureStderr();

                RestoreReportingClient client{ nullptr };
                client.sendRestoreReport(restoreReport1);
                guard1.wait();

                const auto log = internal::GetCapturedStderr();
                EXPECT_THAT(log, HasSubstr(" INFO Reporting successful restoration of /path/eicar1.txt"));
                EXPECT_THAT(log, HasSubstr("DEBUG Correlation ID: 00000000-0000-0000-0000-000000000000"));
                EXPECT_THAT(
                    log,
                    HasSubstr(
                        "DEBUG Path: /path/eicar1.txt, correlation ID: 00000000-0000-0000-0000-000000000000, successful"));
            }
            {
                internal::CaptureStderr();

                RestoreReportingClient client{ nullptr };
                client.sendRestoreReport(restoreReport2);
                guard2.wait();

                const auto log = internal::GetCapturedStderr();
                EXPECT_THAT(log, HasSubstr(" INFO Reporting unsuccessful restoration of /path/eicar2.txt"));
                EXPECT_THAT(log, HasSubstr("DEBUG Correlation ID: 11111111-1111-1111-1111-111111111111"));
                EXPECT_THAT(
                    log,
                    HasSubstr(
                        "DEBUG Path: /path/eicar2.txt, correlation ID: 11111111-1111-1111-1111-111111111111, unsuccessful"));
            }
        }
    }
}

TEST_F(TestRestoreReportSocket, TestSendInvalidReportDoesntPreventValidReportFromBeingReceived)
{
    const scan_messages::RestoreReport invalidRestoreReport{ 0, "/path/eicar.txt", "invalid id", false };
    const scan_messages::RestoreReport validRestoreReport{
        0, "/path/eicar.txt", "00000000-0000-0000-0000-000000000000", false
    };

    MockRestoreReportProcessor mockReporter;
    WaitForEvent guard;
    EXPECT_CALL(mockReporter, processRestoreReport(validRestoreReport))
        .WillOnce(InvokeWithoutArgs([&guard]() { guard.onEventNoArgs(); }));

    {
        RestoreReportingServer server{ mockReporter };
        server.start();

        {
            UsingMemoryAppender memoryAppenderHolder(*this);
            RestoreReportingClient client{ nullptr };
            client.sendRestoreReport(invalidRestoreReport);
            EXPECT_TRUE(waitForLog(
                "Restore Reporting Server failed to receive a report: Correlation ID is not a valid UUID", 100ms));
        }

        {
            RestoreReportingClient client{ nullptr };
            client.sendRestoreReport(validRestoreReport);
        }

        guard.wait();
    }
}

TEST_F(TestRestoreReportSocket, TestClientTimesOut)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    RestoreReportingClient client{ nullptr };

    EXPECT_TRUE(appenderContains("Failed to connect to Restore Reporting Server - retrying after sleep"));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}

TEST_F(TestRestoreReportSocket, TestClientTimeOutInterrupted)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::Threads::NotifyPipe notifyPipe;

    auto notifyPipeSleeper = std::make_unique<common::NotifyPipeSleeper>(notifyPipe);

    std::thread t1([&notifyPipeSleeper]() { RestoreReportingClient client{ std::move(notifyPipeSleeper) }; });

    EXPECT_TRUE(waitForLog("Failed to connect to Restore Reporting Server - retrying after sleep", 2s));
    notifyPipe.notify();

    t1.join();

    EXPECT_TRUE(appenderContains("Stop requested while connecting to Restore Reporting Server"));
    EXPECT_FALSE(appenderContains("Reached total maximum number of connection attempts."));
}

TEST_F(TestRestoreReportSocket, TestConcurrentConnection)
{
    const scan_messages::RestoreReport restoreReport1{
        0, "/path/eicar1.txt", "00000000-0000-0000-0000-000000000000", true
    };

    const scan_messages::RestoreReport restoreReport2{
        0, "/path/eicar2.txt", "11111111-1111-1111-1111-111111111111", false
    };

    MockRestoreReportProcessor mockReporter;
    WaitForEvent guardReceiveFirstReport;
    WaitForEvent guardSendSecondReport;
    WaitForEvent guard;
    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport1))
        .WillOnce(InvokeWithoutArgs(
            [&guardReceiveFirstReport, &guardSendSecondReport]()
            {
                guardReceiveFirstReport.onEventNoArgs();
                guardSendSecondReport.wait();
            }));

    // processRestoreReport is not called on restoreReport2

    RestoreReportingServer server{ mockReporter };
    server.start();

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport1);
    }

    guardReceiveFirstReport.wait();

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport2);
        // Need to sleep until we can be sure the socket communication was attempted
        std::this_thread::sleep_for(std::chrono::seconds(1));
        guardSendSecondReport.onEventNoArgs();
    }
}
