/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatReporter.h"

#include <utility>

#include "Logger.h"

#include "sophos_threat_detector/threat_scanner/ScannerInfo.h"

#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

namespace fs = sophos_filesystem;

sspl::sophosthreatdetectorimpl::ThreatReporter::ThreatReporter(
    sspl::sophosthreatdetectorimpl::ThreatReporter::path  threatReporterSocketPath)
    : m_threatReporterSocketPath(std::move(threatReporterSocketPath))
{
}

void sspl::sophosthreatdetectorimpl::ThreatReporter::sendThreatReport(
    const std::string& threatPath,
    const std::string& threatName,
    const std::string& sha256,
    int64_t scanType,
    const std::string& userID,
    std::time_t detectionTimeStamp)
{
    if (threatPath.empty())
    {
        LOGERROR("Missing path while sending Threat Report: empty string");
    }

    try
    {
        fs::path threatReporterSocketPath = m_threatReporterSocketPath;
        LOGDEBUG("Threat reporter path " << threatReporterSocketPath);
        unixsocket::ThreatReporterClientSocket threatReporterSocket(threatReporterSocketPath);

        scan_messages::ThreatDetected threatDetected;
        threatDetected.setUserID(userID);
        threatDetected.setDetectionTime(detectionTimeStamp);
        threatDetected.setScanType(static_cast<scan_messages::E_SCAN_TYPE>(scanType));
        // For now this is always 1 (Virus)
        threatDetected.setThreatType(scan_messages::E_VIRUS_THREAT_TYPE);
        threatDetected.setThreatName(threatName);
        threatDetected.setNotificationStatus(scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE);
        threatDetected.setFilePath(threatPath);
        threatDetected.setActionCode(scan_messages::E_SMT_THREAT_ACTION_NONE);
        threatDetected.setSha256(sha256);

        threatReporterSocket.sendThreatDetection(threatDetected);
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to send threat report: " << e.what());
    }
}