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

    // todo remove with LINUXDAR-6861
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
                LOGINFO("Allowing " << common::escapePathForLogging(detection.path) << " as PUA detection is disabled");
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
                "Detected \"" << detection.name << "\" in " << common::escapePathForLogging(detection.path) << " ("
                              << scanTypeStr << ")");
        }

        response.addDetection(detection.path, detection.type, detection.name, detection.sha256);
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

scan_messages::MetadataRescanResponse SusiScanner::metadataRescan(const scan_messages::MetadataRescan& request)
{
    std::string escapedPath{ common::escapePathForLogging(request.filePath) };

    auto result = metadataRescanInner(request, escapedPath);

    LOGDEBUG(
        "Metadata rescan of quarantined file (original path '"
        << escapedPath << "') has result: " << MetadataRescanResponseToString(result));

    return result;
}

scan_messages::MetadataRescanResponse SusiScanner::metadataRescanInner(
    const scan_messages::MetadataRescan& request,
    std::string_view escapedPath)
{
    m_shutdownTimer->reset();

    LOGDEBUG("Starting metadata rescan of quarantined file (original path '" << escapedPath << "')");

    // First rescan the threat
    LOGDEBUG("Performing metadata rescan on the main threat");
    auto resultForDetection = m_unitScanner->metadataRescan(
        request.threat.type, request.threat.name, request.filePath, request.threat.sha256);
    LOGDEBUG("Main threat has rescan result: " << MetadataRescanResponseToString(resultForDetection));

    scan_messages::MetadataRescanResponse resultForWholeFile;

    // If the hash of the file is different from that of the threat, rescan the whole file's SHA256 as well.
    // In that case we don't have the threatType or threatName available so use 'unknown' for both
    // This is necessary to determine if an archive has been allowlisted or suppressed as a whole
    if (request.sha256 != request.threat.sha256)
    {
        LOGDEBUG("Performing metadata rescan on the file's SHA256");
        resultForWholeFile = m_unitScanner->metadataRescan("unknown", "unknown", request.filePath, request.sha256);
        LOGDEBUG("File's SHA256 has rescan result: " << MetadataRescanResponseToString(resultForWholeFile));
    }
    else
    {
        LOGDEBUG("Threat SHA256 matches file SHA256, reusing threat rescan result for whole file");
        resultForWholeFile = resultForDetection;
    }

    switch (resultForWholeFile)
    {
        case scan_messages::MetadataRescanResponse::clean:
        case scan_messages::MetadataRescanResponse::threatPresent:
        case scan_messages::MetadataRescanResponse::needsFullScan:
        case scan_messages::MetadataRescanResponse::failed:
            return resultForWholeFile;
        case scan_messages::MetadataRescanResponse::undetected:
            break;
    }

    switch (resultForDetection)
    {
        case scan_messages::MetadataRescanResponse::clean:
        case scan_messages::MetadataRescanResponse::undetected:
            // These two mean that the provided detection is no longer there.
            // Since the rescan request MetadataRescan doesn't have all detections, we don't know if there are any other
            // detections present, so we must request a full rescan.
            resultForDetection = scan_messages::MetadataRescanResponse::needsFullScan;
            break;
        case scan_messages::MetadataRescanResponse::needsFullScan:
        case scan_messages::MetadataRescanResponse::threatPresent:
        case scan_messages::MetadataRescanResponse::failed:
            break;
    }

    return resultForDetection;
}
