// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "SusiScanner.h"

#include "Logger.h"
#include "SusiScanResultJsonParser.h"
#include "ThreatDetectedBuilder.h"

#include "common/StringUtils.h"
#include "scan_messages/ClientScanRequest.h"

using namespace scan_messages;
using namespace threat_scanner;
using namespace common::CentralEnums;

scan_messages::ScanResponse SusiScanner::scan(
    datatypes::AutoFd& fd,
    const std::string& file_path,
    int64_t scanType,
    const std::string& userID)
{
    m_shutdownTimer->reset();

    const ScanResult result = m_unitScanner->scan(fd, file_path);

    scan_messages::ScanResponse response;

    // Log any errors
    for (auto it = result.errors.begin(); it != result.errors.end(); ++it)
    {
        const auto& error = *it;
        getThreatScannerLogger().log(error.logLevel, error.message);

        // Include the first error in the response
        if (it == result.errors.begin())
        {
            response.setErrorMsg(error.message);
        }
    }

    const auto e_ScanType = static_cast<E_SCAN_TYPE>(scanType);
    const auto scanTypeStr = getScanTypeAsStr(e_ScanType);

    for (const auto& detection : result.detections)
    {
        LOGWARN(
            "Detected \"" << detection.name << "\" in " << common::escapePathForLogging(detection.path) << " ("
                          << scanTypeStr << ")");
        response.addDetection(detection.path, detection.name, detection.sha256);
    }

    if (!result.detections.empty())
    {
        const auto threatDetected =
            buildThreatDetected(result.detections, file_path, std::move(fd), userID, e_ScanType);

        const std::string escapedPath = common::escapePathForLogging(file_path);
        LOGINFO("Sending report for detection '" << threatDetected.threatName << "' in " << escapedPath);

        assert(m_threatReporter);
        m_threatReporter->sendThreatReport(threatDetected);
    }

    return response;
}