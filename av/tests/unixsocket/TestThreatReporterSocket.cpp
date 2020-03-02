/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>

#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "tests/common/Common.h"
#include "datatypes/sophos_filesystem.h"

#include <unistd.h>

#define BASE "/tmp/TestPluginAdapter"

using namespace scan_messages;
namespace fs = sophos_filesystem;

class TestThreatReposterSocket : public ::testing::Test
{
public:
    void SetUp() override
    {
        setupFakeSophosThreatReporterConfig();
    }

    void TearDown() override
    {
        fs::remove_all("/tmp/TestPluginAdapter/");
    }

};

TEST_F(TestThreatReposterSocket, TestSendThreatReport) // NOLINT
{
    std::string socketPath = "/tmp/TestPluginAdapter/chroot/threat_report_socket";
    std::string threatName = "unit-test-eicar";
    std::string threatPath = "/path/to/unit-test-eicar";
    std::time_t detectionTimeStamp = std::time(nullptr);
    std::string userID = std::getenv("USER");
    unixsocket::ThreatReporterServerSocket threatReporterServer(socketPath);

    threatReporterServer.start();

    // connect after we start
    unixsocket::ThreatReporterClientSocket threatReporterSocket(socketPath);

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