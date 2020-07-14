/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScanner.h"

#include "Logger.h"
#include "ScannerInfo.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iostream>
#include <string>

using namespace threat_scanner;
using json = nlohmann::json;

namespace fs = sophos_filesystem;

static fs::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}

static fs::path threat_reporter_socket()
{
    return pluginInstall() / "chroot/threat_report_socket";
}

static fs::path susi_library_path()
{
    return pluginInstall() / "chroot/susi/distribution_version";
}

static std::string create_runtime_config(const std::string& scannerInfo)
{
    fs::path libraryPath = susi_library_path();
    std::string runtimeConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SSPL AV Plugin",
            "context": "File",
            "version": "1.0.0"
        },
        "customerID": "0123456789abcdef",
        "machineID": "fedcba9876543210"
    },
    @@SCANNER_CONFIG@@
})sophos", {{"@@LIBRARY_PATH@@", libraryPath},
            {"@@SCANNER_CONFIG@@", scannerInfo}
    });
    return runtimeConfig;
}

static std::string create_scanner_config(const std::string& scannerInfo)
{
    return "{"+scannerInfo+"}";
}

SusiScanner::SusiScanner(const std::shared_ptr<ISusiWrapperFactory>& susiWrapperFactory, bool scanArchives)
{
    std::string scannerInfo = create_scanner_info(scanArchives);

    std::string runtimeConfig = create_runtime_config(scannerInfo);
    std::string scannerConfig = create_scanner_config(scannerInfo);

    m_susi = susiWrapperFactory->createSusiWrapper(runtimeConfig, scannerConfig);
}

void SusiScanner::sendThreatReport(
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
SusiScanner::scan(
    datatypes::AutoFd& fd,
    const std::string& file_path,
    int64_t scanType,
    const std::string& userID)
{
    scan_messages::ScanResponse response;
    response.setClean(true);
    response.setThreatName("unknown");

    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    SusiScanResult* scanResult = nullptr;
    SusiResult res = m_susi->scanFile(metaDataJson.c_str(), file_path.c_str(), fd, &scanResult);

    LOGINFO("Scan result " << std::hex << res << std::dec);
    if (scanResult != nullptr)
    {
        try
        {
            LOGINFO("Details: " << scanResult->version << ", " << scanResult->scanResultJson);
            std::string scanResultUTF8 = common::toUtf8(scanResult->scanResultJson, false);

            LOGINFO("Converted: " << scanResultUTF8);

            json parsedScanResult = json::parse(scanResultUTF8);
            for (auto result : parsedScanResult["results"])
            {
                for (auto detection : result["detections"])
                {
                    LOGERROR("Detected " << detection["threatName"] << " in " << detection["path"]);
                    response.setThreatName(detection["threatName"]);
                    response.setFullScanResult(scanResultUTF8);
                }
            }
        }
        catch (const std::exception& e)
        {
            // CORE-1517 - Until SUSI responses are always valid JSON
            LOGERROR("Failed to parse response from SUSI: " << e.what());
        }
    }

    m_susi->freeResult(scanResult);

    if (res == SUSI_I_THREATPRESENT)
    {
        response.setClean(false);
        sendThreatReport(file_path, response.threatName(), scanType, userID);
    }

    return response;
}