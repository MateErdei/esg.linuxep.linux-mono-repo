// Copyright 2022 Sophos Limited. All rights reserved.

#include "ThreatDetectedBuilder.h"

#include "Logger.h"
#include "SusiResultUtils.h"

#include "common/StringUtils.h"

#include <Common/FileSystem/IFileSystem.h>

namespace threat_scanner
{
    // TODO LINUXDAR-5793: Implement correct method of generating threatId
    std::string generateThreatId(const std::string& filePath, const std::string& threatName)
    {
        std::string threatId = "T" + common::sha256_hash(filePath + threatName);
        return threatId.substr(0, 16);
    }

    threat_scanner::Detection getMainDetection(
        const std::vector<threat_scanner::Detection>& detections,
        const std::string& path,
        int fd)
    {
        if (detections.empty())
        {
            // Precondition failed, it is simpler to return a dummy result here than throw
            LOGWARN("Trying to get the main detection when no detections were provided");
            return { path, "unknown", "unknown", "unknown" };
        }

        // First try to find a detection that matches the provided path
        for (const auto& detection : detections)
        {
            if (detection.path == path)
            {
                LOGDEBUG("Found detection from SUSI that matches scanned path so using it: " << detection.sha256);
                return detection;
            }
            else
            {
                LOGDEBUG("Rejecting detection of " << detection.path << " with sha256 of " << detection.sha256);
            }
        }

        // Otherwise we need to create a new detection for the requested path.
        // As we have no way of knowing the SHA256 of the requested path, we will try to calculate it.
        threat_scanner::Detection mainDetection{ path, detections.at(0).name, detections.at(0).type, "unknown" };

        std::string escapedPath = common::escapePathForLogging(path);

        try
        {
            mainDetection.sha256 =
                Common::FileSystem::fileSystem()->calculateDigest(Common::SslImpl::Digest::sha256, fd);
            LOGDEBUG("Calculated the SHA256 of " << escapedPath << ": " << mainDetection.sha256);
        }
        catch (const std::exception& e)
        {
            LOGWARN("Failed to calculate the SHA256 of " << escapedPath << ": " << e.what());
        }

        return mainDetection;
    }

    scan_messages::ThreatDetected buildThreatDetected(
        const std::vector<threat_scanner::Detection>& detections,
        const std::string& path,
        datatypes::AutoFd autoFd,
        const std::string& userId,
        scan_messages::E_SCAN_TYPE scanType)
    {
        Detection detection = getMainDetection(detections, path, autoFd.get());

        return { userId,
                 std::time(nullptr),
                 detection.type,
                 detection.name,
                 scanType,
                 common::CentralEnums::QuarantineResult::WHITELISTED, // used as placeholder
                 path,
                 detection.sha256,
                 generateThreatId(detection.path, detection.sha256),
                 false, // AV plugin will set this to correct value
                 getReportSource(detection.name),
                 -1,
                 "",
                 "",
                 std::move(autoFd) };
    }
} // namespace threat_scanner