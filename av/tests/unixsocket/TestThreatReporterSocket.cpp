/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/WaitForEvent.h"

#include <fstream>
#include <unistd.h>
#include <unixsocket/threatReporterSocket/ThreatReporterClient.h>

#define BASE "/tmp/TestPluginAdapter"

using namespace scan_messages;
using namespace testing;
namespace fs = sophos_filesystem;

namespace
{
    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(const std::string& threatDetectedXML));
    };
}

void setupFakeSophosThreatReporterConfig()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", BASE);
    fs::path f = BASE;
    fs::create_directories(f / "chroot");
    f /= "sbin";
    fs::create_directories(f);
    f /= "sophos_threat_detector_launcher";
    std::ofstream ost(f);
    ost.close();
}

TEST(TestThreatReposterSocket, TestSendThreatReport) // NOLINT
{
    setupFakeSophosThreatReporterConfig();
    WaitForEvent serverWaitGuard;

    std::string threatName = "unit-test-eicar";
    std::string threatPath = "/path/to/unit-test-eicar";
    std::time_t detectionTimeStamp = std::time(nullptr);
    std::string userID = std::getenv("USER");

    std::shared_ptr<StrictMock<MockIThreatReportCallbacks> > mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();

    EXPECT_CALL(*mock_callback, processMessage(_)).Times(1).WillOnce(
            InvokeWithoutArgs(&serverWaitGuard, &WaitForEvent::onEventNoArgs));;

    unixsocket::ThreatReporterServerSocket threatReporterServer(
            "/tmp/TestPluginAdapter/chroot/threat_report_socket",
            mock_callback);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(
            "/tmp/TestPluginAdapter/chroot/threat_report_socket");

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    threatReporterSocket.sendThreatDetection(threatDetected);

    serverWaitGuard.wait();
    threatReporterServer.requestStop();
    threatReporterServer.join();
}