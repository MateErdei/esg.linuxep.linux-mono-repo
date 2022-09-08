/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "UnixSocketMemoryAppenderUsingTests.h"

#include <unixsocket/threatReporterSocket/ThreatReporterClient.h>

using namespace scan_messages;
using namespace testing;
namespace fs = sophos_filesystem;

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
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        std::string m_threatPath;
        std::string m_threatName;
        std::string m_userID;
        std::string m_socketPath;
    };

    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(const scan_messages::ServerThreatDetected& detection));
    };
}

TEST_F(TestThreatReporterSocket, TestSendThreatReport) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    {
        setupFakeSophosThreatDetectorConfig();
        WaitForEvent serverWaitGuard;

        std::time_t detectionTimeStamp = std::time(nullptr);

        auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

        EXPECT_CALL(*mockThreatReportCallback, processMessage(_)).Times(1).WillOnce(
                InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

        unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);

        threatReporterServer.start();

        // connect after we start
        unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

        scan_messages::ThreatDetected threatDetected;
        threatDetected.setUserID(m_userID);
        threatDetected.setDetectionTime(detectionTimeStamp);
        threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS_OPEN);
        threatDetected.setThreatName(m_threatName);
        threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
        threatDetected.setFilePath(m_threatPath);
        threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

        threatReporterSocket.sendThreatDetection(threatDetected);

        serverWaitGuard.wait();
        //destructor will stop the thread
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

    EXPECT_CALL(*mockThreatReportCallback, processMessage(_)).Times(2).WillOnce(
            InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs)).WillOnce(
            InvokeWithoutArgs(&serverWaitGuard2, &WaitForEvent::onEventNoArgs));

    unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mockThreatReportCallback);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(m_userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS_OPEN);
    threatDetected.setThreatName(m_threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(m_threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    threatReporterSocket.sendThreatDetection(threatDetected);
    serverWaitGuard.wait();

    threatReporterSocket.sendThreatDetection(threatDetected);
    serverWaitGuard2.wait();
    //destructor will stop the thread
}

TEST_F(TestThreatReporterSocket, testClientSocketTriesToReconnect) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    {
        // Attempt to reconnect - it should stop after 9 attempts
        unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath, {0,0});
    }

    EXPECT_TRUE(appenderContains("Failed to connect to Threat reporter - retrying after sleep", 9));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}
