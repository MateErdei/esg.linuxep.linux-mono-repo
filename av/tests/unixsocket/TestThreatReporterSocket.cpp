// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unixsocket/threatReporterSocket/ThreatReporterClient.h>

#include <sys/fcntl.h>

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
            m_userID = std::getenv("USER");
            m_threatName = "unit-test-eicar";
            m_threatPath = "/path/to/unit-test-eicar";
            m_sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";
            m_threatId = "c1c802c6-a878-ee05-babc-c0378d45d8d4";
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        std::string m_threatPath;
        std::string m_threatName;
        std::string m_userID;
        std::string m_socketPath;
        std::string m_sha256;
        std::string m_threatId;
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

        std::time_t detectionTimeStamp = std::time(nullptr);

        auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

        EXPECT_CALL(*mockThreatReportCallback, processMessage(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

        unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);

        threatReporterServer.start();

        // connect after we start
        unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

        scan_messages::ThreatDetected threatDetected(
            m_userID,
            detectionTimeStamp,
            ThreatType::virus,
            m_threatName,
            E_SCAN_TYPE_ON_ACCESS_OPEN,
            E_NOTIFICATION_STATUS_CLEANED_UP,
            m_threatPath,
            E_SMT_THREAT_ACTION_SHRED,
            m_sha256,
            m_threatId,
            false,
            ReportSource::ml,
            datatypes::AutoFd(open("/dev/zero", O_RDONLY)));

        threatReporterSocket.sendThreatDetection(threatDetected);

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

    std::time_t detectionTimeStamp = std::time(nullptr);

    auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*mockThreatReportCallback, processMessage(_))
        .Times(2)
        .WillOnce(InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs))
        .WillOnce(InvokeWithoutArgs(&serverWaitGuard2, &WaitForEvent::onEventNoArgs));

    unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

    scan_messages::ThreatDetected threatDetected(
        m_userID,
        detectionTimeStamp,
        ThreatType::virus,
        m_threatName,
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        m_threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        m_sha256,
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd(open("/dev/zero", O_RDONLY)));

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
        unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath, std::chrono::seconds { 0 });
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

    scan_messages::ThreatDetected threatDetected(
        m_userID,
        std::time(nullptr),
        ThreatType::virus,
        m_threatName,
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        m_threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        m_sha256,
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd(open("/dev/zero", O_RDONLY)));
    threatDetected.threatId = "invalid threat id";

    unixsocket::ThreatReporterClientSocket client(m_socketPath);
    EXPECT_ANY_THROW(client.sendThreatDetection(threatDetected));
}