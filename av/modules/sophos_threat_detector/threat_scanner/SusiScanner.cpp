// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "SusiScanner.h"

#include "Logger.h"
#include "SusiScanResultJsonParser.h"
#include "ThreatDetectedBuilder.h"

#include "common/StringUtils.h"
#include "scan_messages/ClientScanRequest.h"
#include "scan_messages/ThreatDetected.h"

#include <iostream>
#include <string>

using namespace scan_messages;
using namespace threat_scanner;
using namespace common::CentralEnums;

scan_messages::ScanResponse SusiScanner::scan(
    datatypes::AutoFd& fd,
    const scan_messages::ScanRequest& info)
{
    m_shutdownTimer->reset();

    const ScanResult result = m_unitScanner->scan(fd, info.getPath());

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

    const auto e_ScanType = static_cast<E_SCAN_TYPE>(info.getScanType());
    const auto scanTypeStr = getScanTypeAsStr(e_ScanType);

    for (const auto& detection : result.detections)
    {
        if (e_ScanType != E_SCAN_TYPE_SAFESTORE_RESCAN)
        {
            LOGWARN(
                "Detected \"" << detection.name << "\" in " << common::escapePathForLogging(detection.path) << " ("
                              << scanTypeStr << ")");
        }

        response.addDetection(detection.path, detection.name, detection.sha256);
    }

    if (!result.detections.empty() && e_ScanType != E_SCAN_TYPE_SAFESTORE_RESCAN)
    {
        auto threatDetected =
            buildThreatDetected(result.detections, info.getPath(), std::move(fd), info.getUserId(), e_ScanType);
        if (e_ScanType == E_SCAN_TYPE_ON_ACCESS || e_ScanType == E_SCAN_TYPE_ON_ACCESS_CLOSE || e_ScanType == E_SCAN_TYPE_ON_ACCESS_OPEN )
        {
            threatDetected.pid = info.getPid();
            threatDetected.executablePath = info.getExecutablePath();
        }
        const std::string escapedPath = common::escapePathForLogging(info.getPath());
        LOGINFO("Sending report for detection '" << threatDetected.threatName << "' in " << escapedPath);

        LOGDEBUG("Threat ID: " << threatDetected.threatId);

        assert(m_threatReporter);
        m_threatReporter->sendThreatReport(threatDetected);
    }

    return response;
}