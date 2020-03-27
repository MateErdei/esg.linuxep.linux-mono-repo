/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanClient.h"

#include "Logger.h"

#include "common/Define.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <iostream>
#include <fcntl.h>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

ScanClient::ScanClient(unixsocket::IScanningClientSocket& socket,
                       std::shared_ptr<IScanCallbacks> callbacks,
                       bool scanInArchives,
                       E_SCAN_TYPE scanType)
        : m_socket(socket)
        , m_callbacks(std::move(callbacks))
        , m_scanInArchives(scanInArchives)
        , m_scanType(scanType)
{
}

static fs::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    try
    {
        return appConfig.getData("PLUGIN_INSTALL");
    }
    catch (const std::out_of_range&)
    {
        return "/opt/sophos-spl/plugins/av";
    }

}

static fs::path threat_reporter_socket()
{
#ifdef USE_CHROOT
    return pluginInstall() / "chroot/threat_report_socket";
#else
    return pluginInstall() / "var/threat_report_socket";
#endif
}

void ScanClient::sendThreatReport(const fs::path& threatPath, const std::string& threatName)
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
    const char* user = std::getenv("USER");
    threatDetected.setUserID(user ? user : "root");
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(m_scanType);
    //For now this is always 1 (Virus)
    threatDetected.setThreatType(E_VIRUS_THREAT_TYPE);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_NOT_CLEANUPABLE);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_NONE);

    threatReporterSocket.sendThreatDetection(threatDetected);
}

scan_messages::ScanResponse ScanClient::scan(const sophos_filesystem::path& fileToScanPath)
{
    datatypes::AutoFd file_fd(::open(fileToScanPath.c_str(), O_RDONLY));
    if (!file_fd.valid())
    {
        LOGERROR("Unable to open "<< fileToScanPath);
        return scan_messages::ScanResponse();
    }

    scan_messages::ClientScanRequest request;
    request.setPath(fileToScanPath);
    request.setScanInsideArchives(m_scanInArchives);

    auto response = m_socket.scan(file_fd, request);

    if (m_callbacks)
    {
        if (response.clean())
        {
            m_callbacks->cleanFile(fileToScanPath);
        }
        else
        {
            sendThreatReport(fileToScanPath, response.threatName());
            m_callbacks->infectedFile(fileToScanPath, response.threatName());
        }
    }
    return response;
}
