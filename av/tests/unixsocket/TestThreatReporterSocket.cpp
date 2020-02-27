/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "datatypes/sophos_filesystem.h"

#include <fstream>

#include <unixsocket/threatReporterSocket/ThreatReporterClient.h>

#define BASE "/tmp/TestPluginAdapter"

using namespace scan_messages;
namespace fs = sophos_filesystem;

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
#include <unistd.h>

TEST(TestThreatReposterSocket, TestSendThreatReport) // NOLINT
{
    setupFakeSophosThreatReporterConfig();

    std::string threatName = "unit-test-eicar";
    std::string threatPath = "/path/to/unit-test-eicar";
    std::time_t detectionTimeStamp = std::time(nullptr);
    std::string userID = std::getenv("USER");
    unixsocket::ThreatReporterServerSocket threatReporterServer(
            "/tmp/TestPluginAdapter/chroot/threat_report_socket"
    );

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket("/tmp/TestPluginAdapter/chroot/threat_report_socket");

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    threatReporterSocket.sendThreatDetection(threatDetected);
    //TO DO: check that a callback to the avpmanager is called [LINUXDAR-1473 LINUXDAR-1470]
    //follow test scan client example
    //communication to the socket takes some time this makes sure we don't kill the socket too fast
    //when the TO DO will get implemented a timeout will be put in place instead of the sleep
    usleep(100000);
    threatReporterServer.requestStop();
    threatReporterServer.join();
}