//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

#include <string>
#include <ctime>

using namespace scan_messages;

int main()
{
    std::string threatName = "test-eicar";
    std::string threatPath = "/path/to/test-eicar";
    std::time_t detectionTimeStamp = std::time(nullptr);
    std::string userID = std::getenv("USER");

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ThreatReporterClientSocket socket(path);

    scan_messages::ThreatDetected threatDetected;

    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS_OPEN);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    socket.sendThreatDetection(threatDetected);

    return 0;
}
