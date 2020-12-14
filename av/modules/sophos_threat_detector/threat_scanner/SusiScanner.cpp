/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScanner.h"

#include "Logger.h"
#include "ScannerInfo.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "pluginimpl/ObfuscationImpl/Base64.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iostream>
#include <string>

using namespace threat_scanner;
using json = nlohmann::json;

namespace fs = sophos_filesystem;

static fs::path threat_reporter_socket()
{
    return pluginInstall() / "chroot/var/threat_report_socket";
}

static std::string create_scanner_config(const std::string& scannerInfo)
{
    return "{"+scannerInfo+"}";
}

SusiScanner::SusiScanner(const std::shared_ptr<ISusiWrapperFactory>& susiWrapperFactory, bool scanArchives)
{
    std::string scannerInfo = create_scanner_info(scanArchives);
    std::string scannerConfig = create_scanner_config(scannerInfo);
    m_susi = susiWrapperFactory->createSusiWrapper(scannerConfig);
}

void SusiScanner::sendThreatReport(
        const std::string& threatPath,
        const std::string& threatName,
        int64_t scanType,
        const std::string& userID)
{
    if (threatPath.empty())
    {
        LOGERROR("Missing path while sending Threat Report: empty string");
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
SusiScanner::scan(
        datatypes::AutoFd& fd,
        const std::string& file_path,
        int64_t scanType,
        const std::string& userID)
{
    scan_messages::ScanResponse response;

    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    SusiScanResult* scanResult = nullptr;
    SusiResult res = m_susi->scanFile(metaDataJson.c_str(), file_path.c_str(), fd, &scanResult);

    LOGTRACE("Scanning " << file_path.c_str() << " result: " << std::hex << res << std::dec);
    if (scanResult != nullptr)
    {
        try
        {
            std::string scanResultJson = scanResult->scanResultJson;
            LOGDEBUG("Scanning result details: " << scanResult->version << ", " << scanResultJson);
            response.setFullScanResult(scanResultJson);

            json parsedScanResult = json::parse(scanResultJson);
            for (auto result : parsedScanResult["results"])
            {
                std::string utf8Path(common::toUtf8(Common::ObfuscationImpl::Base64::Decode(result["base64path"]), true));
                std::string escapedPath = utf8Path;
                common::escapeControlCharacters(escapedPath);
                for (auto detection : result["detections"])
                {
                    LOGWARN("Detected " << detection["threatName"] << " in " << escapedPath);
                    response.addDetection(utf8Path, detection["threatName"]);
                }
            }
        }
        catch (const std::exception& e)
        {
            // CORE-1517 - Until SUSI responses are always valid JSON
            LOGERROR("Failed to parse SUSI response: " << e.what() << " SUSI Response:" << scanResult->scanResultJson);
        }
    }

    m_susi->freeResult(scanResult);

    if (res == SUSI_I_THREATPRESENT)
    {
        std::vector<std::pair<std::string, std::string>> detections = response.getDetections();
        if (detections.empty())
        {
            // Failed to parse SUSI scan report but the return code shows that we detected a threat
            response.addDetection(file_path, "unknown");
            sendThreatReport(file_path, "unknown", scanType, userID);
        }
        else
        {
            for (const auto& detection: detections)
            {
                sendThreatReport(detection.first, detection.second, scanType, userID);
            }
        }
    }
    else if (res == SUSI_E_SCANABORTED)
    {
        // Return codes that cover zip bombs, corrupted files and password-protected files
        std::stringstream errorMsg;
        errorMsg << "Scanning of " << common::toUtf8(file_path) << " was aborted";
        response.setErrorMsg(errorMsg.str());
    }

    return response;
}
