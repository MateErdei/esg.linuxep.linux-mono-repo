/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeSusiScanner.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

#include <iostream>
#include <string>
#include <unistd.h>

#include <sys/stat.h>

using namespace threat_scanner;

namespace fs = sophos_filesystem;

fs::path fakePluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}

static fs::path threat_reporter_socket()
{
    return fakePluginInstall() / "chroot/threat_report_socket";
}

void FakeSusiScanner::sendThreatReport(
    const std::string& threatPath,
    const std::string& threatName,
    int64_t scanType,
    const std::string& userID)
{
    if (threatPath.empty())
    {
        LOGERROR("ERROR: sendThreatReport with empty path!");
    }

    fs::path threatReporterSocketPath = threat_reporter_socket();
    LOGDEBUG("Threat reporter path " << threatReporterSocketPath);
    unixsocket::ThreatReporterClientSocket threatReporterSocket(threatReporterSocketPath);
    std::time_t detectionTimeStamp = std::time(nullptr);

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(static_cast<scan_messages::E_SCAN_TYPE>(scanType));
    //For now this is always 1 (Virus)
    threatDetected.setThreatType(scan_messages::E_VIRUS_THREAT_TYPE);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(scan_messages::E_SMT_THREAT_ACTION_NONE);

    threatReporterSocket.sendThreatDetection(threatDetected);
}

scan_messages::ScanResponse
FakeSusiScanner::scan(
    datatypes::AutoFd& fd,
    const std::string& file_path,
    int64_t scanType,
    const std::string& userID)
{
    char buffer[512];

    // Test reading the file
    ssize_t bytesRead = read(fd.get(), buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = 0;
    std::cout << "Read " << bytesRead << " from " << file_path << '\n';

    // Test stat the file
    struct stat statbuf = {};
    ::fstat(fd.get(), &statbuf);
    std::cout << "size:" << statbuf.st_size << '\n';

    scan_messages::ScanResponse response;
    std::string contents(buffer);
    bool clean = (contents.find("EICAR") == std::string::npos);
    response.setClean(clean);
    if (!clean)
    {
        std::string threatName = "EICAR";
        response.setThreatName(threatName);
        sendThreatReport(file_path, threatName, scanType, userID);
    }

    return response;
}
