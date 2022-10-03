// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

#include <string>
#include <ctime>
#include <fcntl.h>

using namespace scan_messages;

int main()
{
    std::string threatName = "test-eicar";
    std::string threatPath = "/path/to/test-eicar";
    std::time_t detectionTimeStamp = std::time(nullptr);
    std::string userID = std::getenv("USER");
    std::string sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";
    std::string threatId = "Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350";

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ThreatReporterClientSocket socket(path);

    scan_messages::ThreatDetected threatDetected(
        userID,
        detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        threatName,
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        sha256,
        threatId,
        datatypes::AutoFd(::open("/dev/null", O_RDONLY)));

    socket.sendThreatDetection(threatDetected);

    return 0;
}
