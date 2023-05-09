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

    handleDetections(info, result.detections, fd, response);
    return response;
}

void SusiScanner::handleDetections(
    const ScanRequest& info,
    const std::vector<Detection>& detections,
    datatypes::AutoFd& fd,
    ScanResponse& response)
{
    if (detections.empty())
    {
        return;
    }

    assert(m_globalHandler);

    if (m_globalHandler->isAllowListedPath(info.path()))
    {
        LOGINFO("Allowing " << common::escapePathForLogging(info.getPath()) << " as path is in allow list");
        return;
    }

    // SUSI reported detections
    std::vector<Detection> valid_detections;

    for (const auto& detection : detections)
    {
        if (detection.type == "PUA")
        {
            // PUAs have special exclusions

            // 1st check if PUAs are enabled at all:
            if (!info.detectPUAs())
            {
                // CORE-3404: Until fixed some PUAs will still be detected by SUSI despite the setting being disabled
                LOGINFO(
                    "Allowing " << common::escapePathForLogging(detection.path) << " as PUA detection is disabled");
                continue;
            }
            // Then check policy allow-list:
            else if (m_globalHandler->isPuaApproved(detection.name))
            {
                LOGINFO(
                    "Allowing PUA " << common::escapePathForLogging(detection.path) << " by exclusion '"
                                    << detection.name << "'");
                continue;
            }
            else
            {
                // Then check exclusions from request (command-line)
                auto puaExclusions = info.getPuaExclusions();
                if (std::find(puaExclusions.begin(), puaExclusions.end(), detection.name) != puaExclusions.end())
                {
                    LOGINFO(
                        "Allowing PUA " << common::escapePathForLogging(detection.path) << " by request exclusion '"
                                        << detection.name << "'");
                    continue;
                }
            }
        }

        if (m_globalHandler->isAllowListedSha256(detection.sha256))
        {
            LOGINFO("Allowing " << common::escapePathForLogging(info.getPath()) << " with " << detection.sha256);
            continue;
        }

        valid_detections.emplace_back(detection);
    }

    if (valid_detections.empty())
    {
        // No remaining detections to report - everything has been excluded or allowed
        return;
    }

    const auto e_ScanType = static_cast<E_SCAN_TYPE>(info.getScanType());
    auto threatDetected =
        buildThreatDetected(valid_detections, info.getPath(), std::move(fd), info.getUserId(), e_ScanType);

    if (m_globalHandler->isAllowListedSha256(threatDetected.sha256))
    {
        LOGINFO("Allowing " << common::escapePathForLogging(info.getPath()) << " with " << threatDetected.sha256);
        return;
    }

    const auto scanTypeStr = getScanTypeAsStr(e_ScanType);

    for (const auto& detection : valid_detections)
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
