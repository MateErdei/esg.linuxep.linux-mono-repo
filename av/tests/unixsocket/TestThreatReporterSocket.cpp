/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/Common.h"
#include "tests/common/LogInitializedTests.h"
#include "tests/common/WaitForEvent.h"

#include <unixsocket/threatReporterSocket/ThreatReporterClient.h>

using namespace scan_messages;
using namespace testing;
namespace fs = sophos_filesystem;

namespace
{
    class TestThreatReporterSocket : public LogInitializedTests
    {
    public:
        void SetUp() override
        {
            setupFakeSophosThreatDetectorConfig();
            m_socketPath = pluginInstall() / "chroot/threat_report_socket";
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
        MOCK_METHOD1(processMessage, void(const std::string& threatDetectedXML));
    };
}

TEST_F(TestThreatReporterSocket, TestSendThreatReport) // NOLINT
{
    setupFakeSophosThreatDetectorConfig();
    WaitForEvent serverWaitGuard;


    std::time_t detectionTimeStamp = std::time(nullptr);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*mock_callback, processMessage(_)).Times(1).WillOnce(
            InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));

    unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mock_callback);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(m_userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(m_threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(m_threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    threatReporterSocket.sendThreatDetection(threatDetected);

    serverWaitGuard.wait();
    threatReporterServer.requestStop();
    threatReporterServer.join();
}

TEST_F(TestThreatReporterSocket, TestSendTwoThreatReports) // NOLINT
{
    setupFakeSophosThreatDetectorConfig();
    WaitForEvent serverWaitGuard;
    WaitForEvent serverWaitGuard2;


    std::time_t detectionTimeStamp = std::time(nullptr);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*mock_callback, processMessage(_)).Times(2).WillOnce(
            InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs)).WillOnce(
            InvokeWithoutArgs(&serverWaitGuard2, &WaitForEvent::onEventNoArgs));


    unixsocket::ThreatReporterServerSocket threatReporterServer(m_socketPath, 0600, mock_callback);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(m_socketPath);

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(m_userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(m_threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(m_threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    threatReporterSocket.sendThreatDetection(threatDetected);
    serverWaitGuard.wait();

    threatReporterSocket.sendThreatDetection(threatDetected);
    serverWaitGuard2.wait();

    threatReporterServer.requestStop();
    threatReporterServer.join();
}