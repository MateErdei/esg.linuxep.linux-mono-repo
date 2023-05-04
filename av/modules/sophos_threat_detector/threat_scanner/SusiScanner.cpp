// Copyright 2020-2023 Sophos Limited. All rights reserved.

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

scan_messages::ScanResponse SusiScanner::scan(datatypes::AutoFd& fd, const scan_messages::ScanRequest& info)
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

    if (!result.detections.empty())
    {
        // SUSI reported detections

        const auto e_ScanType = static_cast<E_SCAN_TYPE>(info.getScanType());
        auto threatDetected =
            buildThreatDetected(result.detections, info.getPath(), std::move(fd), info.getUserId(), e_ScanType);

        bool reportDetections = true;
        assert(m_globalHandler);
        if (m_globalHandler->isAllowListedSha256(threatDetected.sha256))
        {
            LOGINFO("Allowing " << common::escapePathForLogging(info.getPath()) << " with " << threatDetected.sha256);
            reportDetections = false;
        }
        //todo remove with LINUXDAR-6861
        else if (m_globalHandler->isAllowListedPath(info.path()))
        {
            LOGINFO("Allowing " << common::escapePathForLogging(info.getPath()) << " as path is in allow list");
            reportDetections = false;
        }
        else if (threatDetected.threatType == "PUA")
        {
            // PUAs have special exclusions

            // 1st check if PUAs are enabled at all:
            if (!info.detectPUAs())
            {
                // CORE-3404: Until fixed some PUAs will still be detected by SUSI despite the setting being disabled
                LOGINFO("Allowing " << common::escapePathForLogging(info.getPath()) << " as PUA detection is disabled");
                reportDetections = false;
            }
            // Then check policy allow-list:
            else if (m_globalHandler->isPuaApproved(threatDetected.threatName))
            {
                LOGINFO(
                    "Allowing PUA " << common::escapePathForLogging(info.getPath()) << " by exclusion '"
                                    << threatDetected.threatName << "'");
                reportDetections = false;
            }
            else
            {
                // Then check exclusions from request (command-line)
                auto puaExclusions = info.getPuaExclusions();
                if (std::find(puaExclusions.begin(), puaExclusions.end(), threatDetected.threatName) != puaExclusions.end())
                {
                    LOGINFO(
                        "Allowing PUA " << common::escapePathForLogging(info.getPath()) << " by request exclusion '"
                                        << threatDetected.threatName << "'");
                    reportDetections = false;
                }
            }
        }

        if (reportDetections)
        {
            const auto scanTypeStr = getScanTypeAsStr(e_ScanType);

            for (const auto& detection : result.detections)
            {
                if (e_ScanType != E_SCAN_TYPE_SAFESTORE_RESCAN)
                {
                    LOGWARN(
                        "Detected \"" << detection.name << "\" in " << common::escapePathForLogging(detection.path)
                                      << " (" << scanTypeStr << ")");
                }

                response.addDetection(detection.path, detection.name, detection.sha256);
            }

            if (e_ScanType == E_SCAN_TYPE_ON_ACCESS || e_ScanType == E_SCAN_TYPE_ON_ACCESS_CLOSE ||
                e_ScanType == E_SCAN_TYPE_ON_ACCESS_OPEN)
            {
                threatDetected.pid = info.getPid();
                threatDetected.executablePath = info.getExecutablePath();
            }

            if (e_ScanType != E_SCAN_TYPE_SAFESTORE_RESCAN)
            {
                // Don't send threat report for safestore rescans
                const std::string escapedPath = common::escapePathForLogging(info.getPath());
                LOGINFO("Sending report for detection '" << threatDetected.threatName << "' in " << escapedPath);
                LOGDEBUG("Threat ID: " << threatDetected.threatId);
                assert(m_threatReporter);
                m_threatReporter->sendThreatReport(threatDetected);
            }
        }
    }

    return response;
}