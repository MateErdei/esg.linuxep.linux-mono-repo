/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanClient.h"

#include "Logger.h"

#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <iostream>
#include <fcntl.h>

using namespace avscanner::avscannerimpl;
using namespace scan_messages;
namespace fs = sophos_filesystem;

ScanClient::ScanClient(unixsocket::IScanningClientSocket& socket,
                       std::shared_ptr<IScanCallbacks> callbacks,
                       NamedScanConfig& config)
       : ScanClient(socket, std::move(callbacks), config.m_scanArchives)
{
}

ScanClient::ScanClient(unixsocket::IScanningClientSocket& socket,
                       std::shared_ptr<IScanCallbacks> callbacks,
                       bool scanInArchives)
        : m_socket(socket), m_callbacks(std::move(callbacks)), m_scanInArchives(scanInArchives)
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
    return pluginInstall() / "chroot/threat_report_socket";
}

static void sendThreatReport(const fs::path& threatPath, const std::string& threatName)
{
    fs::path threatReporterSocketPath = threat_reporter_socket();
    LOGDEBUG("Threat reporter path " << threatReporterSocketPath);
    unixsocket::ThreatReporterClientSocket threatReporterSocket(threatReporterSocketPath);
    std::time_t detectionTimeStamp = std::time(nullptr);

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(std::getenv("USER"));
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

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
