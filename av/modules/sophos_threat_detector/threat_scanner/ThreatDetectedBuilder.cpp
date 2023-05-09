// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "ThreatDetectedBuilder.h"

#include "Logger.h"
#include "SusiResultUtils.h"

#include "common/StringUtils.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/Uuid.h"

namespace threat_scanner
{
    std::string generateThreatId(const std::string& filePath, const std::string& sha256)
    {
        return Common::UtilityImpl::Uuid::CreateVersion5({}, filePath + sha256);
    }

    scan_messages::ThreatDetected buildThreatDetected(
        const std::vector<threat_scanner::Detection>& detections,
        const std::string& path,
        datatypes::AutoFd autoFd,
        const std::string& userId,
        scan_messages::E_SCAN_TYPE scanType)
    {
        assert(!detections.empty());

        std::string sha256 = "unknown";
        Detection mainDetection;
        bool foundMainDetection = false;

        // First try to find a detection that matches the provided path
        for (const auto& detection : detections)
        {
            if (detection.path == path)
            {
                LOGDEBUG("Found detection from SUSI that matches scanned path so using it: " << detection.sha256);
                mainDetection = detection;
                sha256 = detection.sha256;
                foundMainDetection = true;
                break;
            }
        }

        if (!foundMainDetection)
        {
            mainDetection = detections[0];
            std::string escapedPath = common::escapePathForLogging(path);

            try
            {
                sha256 =
                    Common::FileSystem::fileSystem()->calculateDigest(Common::SslImpl::Digest::sha256, autoFd.get());
                LOGDEBUG("Calculated the SHA256 of " << escapedPath << ": " << sha256);
            }
            catch (const std::exception& e)
            {
                LOGWARN("Failed to calculate the SHA256 of " << escapedPath << ": " << e.what());
            }
        }

        return { userId,
                 std::time(nullptr),
                 mainDetection.type,
                 mainDetection.name,
                 scanType,
                 common::CentralEnums::QuarantineResult::WHITELISTED, // used as placeholder
                 path,
                 sha256,
                 mainDetection.sha256,
                 generateThreatId(path, sha256), // threatId generated from whole file's path and hash
                 false,                          // AV plugin will set this to correct value
                 getReportSource(mainDetection.name),
                 -1,
                 "",
                 "",
                 std::move(autoFd) };
    }
} // namespace threat_scanner