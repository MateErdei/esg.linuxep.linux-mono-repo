// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "tests/scan_messages/SampleThreatDetected.h"
#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace scan_messages;
using namespace testing;
namespace fs = sophos_filesystem;
using namespace common::CentralEnums;

namespace
{
    class TestThreatReporterSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSophosThreatDetectorConfig();
            m_socketPath = pluginInstall() / "chroot/var/threat_report_socket";
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        std::string m_socketPath;
    };

    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(scan_messages::ThreatDetected detection));
    };
} // namespace

TEST_F(TestThreatReporterSocket, TestSendThreatReport) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    {
        setupFakeSophosThreatDetectorConfig();
        WaitForEvent serverWaitGuard;

        auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

        EXPECT_CALL(*mockThreatReportCallback, processMessage(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

        unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);

        threatReporterServer.start();

        // connect after we start
        unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

        threatReporterSocket.sendThreatDetection(createThreatDetectedWithRealFd({}));

        serverWaitGuard.wait();
        // destructor will stop the thread
    }

    EXPECT_TRUE(appenderContains("Threat Reporter Server starting listening on socket:"));
    EXPECT_TRUE(appenderContains("Threat Reporter Server accepting connection:"));
    EXPECT_TRUE(appenderContains("Threat Reporter Server thread got connection"));
    EXPECT_TRUE(appenderContains("Closing Threat Reporter Server socket"));
}

TEST_F(TestThreatReporterSocket, TestSendTwoThreatReports) // NOLINT
{
    setupFakeSophosThreatDetectorConfig();
    WaitForEvent serverWaitGuard;
    WaitForEvent serverWaitGuard2;

    auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*mockThreatReportCallback, processMessage(_))
        .Times(2)
        .WillOnce(InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs))
        .WillOnce(InvokeWithoutArgs(&serverWaitGuard2, &WaitForEvent::onEventNoArgs));

    unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

    scan_messages::ThreatDetected threatDetected = createThreatDetectedWithRealFd({});

    threatReporterSocket.sendThreatDetection(threatDetected);
    serverWaitGuard.wait();

    threatReporterSocket.sendThreatDetection(threatDetected);
    serverWaitGuard2.wait();
    // destructor will stop the thread
}

TEST_F(TestThreatReporterSocket, testClientSocketTriesToReconnect) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    {
        // Attempt to reconnect - it should stop after 9 attempts
        unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath, std::chrono::seconds{ 0 });
    }

    EXPECT_TRUE(appenderContains("Failed to connect to Threat reporter - retrying after sleep", 9));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}

TEST_F(TestThreatReporterSocket, TestSendThreatReportWithInvalidData)
{
    // This is a simple test to verify client throws exception on invalid data
    // It is not possible to test the server because BaseServerSocket doesn't expose the socket publicly

    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);
    threatReporterServer.start();

    unixsocket::ThreatReporterClientSocket client(m_socketPath);
    EXPECT_ANY_THROW(client.sendThreatDetection(createThreatDetected({ .threatId = "invalid threat id" })));
}