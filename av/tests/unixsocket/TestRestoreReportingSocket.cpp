// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "common/ApplicationPaths.h"
#include "common/NotifyPipeSleeper.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingClient.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingServer.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestRestoreReportSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSafeStoreConfig();
            m_socketPath = Plugin::getRestoreReportSocketPath();
        }

        void TearDown() override
        {
            sophos_filesystem::remove_all(tmpdir());
        }

        std::string m_socketPath;
    };

    class MockRestoreReportProcessor : public IRestoreReportProcessor
    {
    public:
        MOCK_METHOD(void, processRestoreReport, (const scan_messages::RestoreReport& restoreReport), (const, override));
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

        EXPECT_TRUE(appenderContains("RestoreReportingServer starting listening on socket: "));
        EXPECT_TRUE(appenderContains("RestoreReportingServer got connection with fd "));
        EXPECT_TRUE(appenderContains("RestoreReportingClient reports successful restoration of /path/eicar.txt"));
        EXPECT_TRUE(appenderContains("Correlation ID: 00000000-0000-0000-0000-000000000000"));
        EXPECT_TRUE(appenderContains("RestoreReportingServer ready to receive data"));
        EXPECT_TRUE(appenderContains("RestoreReportingServer received a report"));
        EXPECT_TRUE(appenderContains(
            "Path: /path/eicar.txt, correlation ID: 00000000-0000-0000-0000-000000000000, successful"));

        // This means the connection has closed
        EXPECT_TRUE(waitForLog("RestoreReportingServer awaiting new connection", 100ms));
    }

    EXPECT_TRUE(appenderContains("Closing RestoreReportingServer socket"));
}

TEST_F(TestRestoreReportSocket, TestSendUnsuccessfulRestoreReport)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    const scan_messages::RestoreReport restoreReport{
        0, "/path/eicar.txt", "00000000-0000-0000-0000-000000000000", false
    };

    MockRestoreReportProcessor mockReporter;
    WaitForEvent guard;
    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport))
        .WillOnce(InvokeWithoutArgs([&guard]() { guard.onEventNoArgs(); }));

    RestoreReportingServer server{ mockReporter };
    server.start();

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport);
    }

    guard.wait();

    EXPECT_TRUE(appenderContains("[INFO] RestoreReportingClient reports unsuccessful restoration of /path/eicar.txt"));
    EXPECT_TRUE(appenderContains(("[DEBUG] RestoreReportingServer: Path: /path/eicar.txt, correlation ID: 00000000-0000-0000-0000-000000000000, unsuccessful")));
}

TEST_F(TestRestoreReportSocket, TestSendTwoReports)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

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
                RestoreReportingClient client{ nullptr };
                client.sendRestoreReport(restoreReport1);
                guard1.wait();

                EXPECT_TRUE(appenderContains("[INFO] RestoreReportingClient reports successful restoration of /path/eicar1.txt"));
                EXPECT_TRUE(appenderContains(
                        "[DEBUG] RestoreReportingServer: Path: /path/eicar1.txt, correlation ID: 00000000-0000-0000-0000-000000000000, successful"));
            }
            {
                RestoreReportingClient client{ nullptr };
                client.sendRestoreReport(restoreReport2);
                guard2.wait();

                EXPECT_TRUE(appenderContains("[INFO] RestoreReportingClient reports unsuccessful restoration of /path/eicar2.txt"));
                EXPECT_TRUE(appenderContains(
                        "[DEBUG] RestoreReportingServer: Path: /path/eicar2.txt, correlation ID: 11111111-1111-1111-1111-111111111111, unsuccessful"));
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
            m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));
            RestoreReportingClient client{ nullptr };
            client.sendRestoreReport(invalidRestoreReport);
            EXPECT_TRUE(waitForLog(
                "[ERROR] RestoreReportingServer failed to receive a report: Correlation ID is not a valid UUID", 100ms));
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

    EXPECT_TRUE(appenderContains("RestoreReportingClient failed to connect to " + m_socketPath + " - retrying upto 10 times with a sleep of 1s"));
    EXPECT_TRUE(appenderContains("RestoreReportingClient reached the maximum number of connection attempts"));
}

TEST_F(TestRestoreReportSocket, TestClientTimeOutInterrupted)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::Threads::NotifyPipe notifyPipe;

    auto notifyPipeSleeper = std::make_unique<common::NotifyPipeSleeper>(notifyPipe);

    std::thread t1([&notifyPipeSleeper]() { RestoreReportingClient client{ std::move(notifyPipeSleeper) }; });

    EXPECT_TRUE(waitForLog("RestoreReportingClient failed to connect to " + m_socketPath + " - retrying upto 10 times with a sleep of 1s", 2s));
    notifyPipe.notify();

    t1.join();

    EXPECT_TRUE(appenderContains("RestoreReportingClient received stop request while connecting"));
    EXPECT_FALSE(appenderContains("RestoreReportingClient reached the maximum number of connection attempts"));
}

TEST_F(TestRestoreReportSocket, TestClientsDontBlockAndAllConnectionsGetResolvedInOrder)
{
    const scan_messages::RestoreReport restoreReport1{
        0, "/path/eicar1.txt", "00000000-0000-0000-0000-000000000000", true
    };

    const scan_messages::RestoreReport restoreReport2{
        0, "/path/eicar2.txt", "11111111-1111-1111-1111-111111111111", false
    };

    const scan_messages::RestoreReport restoreReport3{
        0, "/path/eicar3.txt", "22222222-2222-2222-2222-222222222222", false
    };

    MockRestoreReportProcessor mockReporter;

    RestoreReportingServer server{ mockReporter };
    server.start();

    WaitForEvent guard1;
    WaitForEvent guard2;
    WaitForEvent guard3;
    WaitForEvent guard4;

    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport1))
        .WillOnce(InvokeWithoutArgs(
            [&guard1, &guard2]()
            {
                guard1.wait();
                guard2.onEventNoArgs();
            }));

    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport2))
        .WillOnce(InvokeWithoutArgs(
            [&guard2, &guard3]()
            {
                guard2.wait();
                guard3.onEventNoArgs();
            }));

    EXPECT_CALL(mockReporter, processRestoreReport(restoreReport3))
        .WillOnce(InvokeWithoutArgs(
            [&guard3, &guard4]()
            {
                guard3.wait();
                guard4.onEventNoArgs();
            }));

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport1);
    }

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport2);
    }

    {
        RestoreReportingClient client{ nullptr };
        client.sendRestoreReport(restoreReport3);
    }

    guard1.onEventNoArgs();
    guard4.wait();
}