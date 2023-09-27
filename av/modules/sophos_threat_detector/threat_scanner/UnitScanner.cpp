// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "UnitScanner.h"

#include "Logger.h"
#include "SusiLogger.h"
#include "SusiResultUtils.h"
#include "SusiScanResultJsonParser.h"

#include "common/StringUtils.h"

#include "Common/FileSystem/IFileSystem.h"

// Thirdparties
#include <nlohmann/json.hpp>

// System C Headers
#include <sys/syscall.h>
#include <unistd.h>

// Std C++ Headers
#include <iomanip>

namespace threat_scanner
{
    ScanResult UnitScanner::scan(datatypes::AutoFd& fd, const std::string& path)
    {
        HighestLevelRecorder::reset();

        SusiScanResult* susiScanResult = nullptr;

        long tid = syscall(SYS_gettid);
        std::stringstream paddedThreadId;
        paddedThreadId << std::setfill('0') << std::setw(8);
        paddedThreadId << std::hex << tid << std::dec;

        std::string escapedPath = common::escapePathForLogging(path);

        auto timeBeforeScan = common::getSusiStyleTimestamp();
        LOG_SUSI_DEBUG("D " << timeBeforeScan << " T" << paddedThreadId.str() << " Starting scan of " << escapedPath);

        SusiResult res = susi_->scanFile(nullptr, path.c_str(), fd, &susiScanResult);

        auto timeAfterScan = common::getSusiStyleTimestamp();
        LOG_SUSI_DEBUG(
            "D " << timeAfterScan << " T" << paddedThreadId.str() << " Finished scanning " << escapedPath
                 << " result: " << std::hex << res << std::dec);

        LOGTRACE("Scanning " << escapedPath << " result: " << std::hex << res << std::dec);

        ScanResult result;

        if (susiScanResult != nullptr)
        {
            std::string scanResultJson = susiScanResult->scanResultJson;
            LOGDEBUG("Scanning result details: " << susiScanResult->version << ", " << scanResultJson);
            result = parseSusiScanResultJson(scanResultJson);
        }

        susi_->freeResult(susiScanResult);

        // Edge case where SUSI reports a threat is present but no detections were provided/successfully parsed
        if (res == SUSI_I_THREATPRESENT && result.detections.empty())
        {
            result.errors.push_back(
                { "SUSI detected a threat but didn't provide any detections", log4cplus::WARN_LOG_LEVEL });

            Detection detection{ path, "unknown", "unknown", "unknown" };
            try
            {
                detection.sha256 =
                    Common::FileSystem::fileSystem()->calculateDigest(Common::SslImpl::Digest::sha256, fd.get());
                LOGDEBUG("Calculated the SHA256 of " << escapedPath << ": " << detection.sha256);
            }
            catch (const std::exception& e)
            {
                result.errors.push_back({ "Failed to calculate the SHA256 of " + escapedPath + ": " + e.what(),
                                          log4cplus::WARN_LOG_LEVEL });
            }

            result.detections.push_back(std::move(detection));
        }

        if (SUSI_FAILURE(res))
        {
            log4cplus::LogLevel logLevel = log4cplus::ERROR_LOG_LEVEL;
            std::string errorMsg = susiResultErrorToReadableError(escapedPath, res, logLevel);
            result.errors.push_back({ errorMsg, logLevel });
        }

        // Report if there are no scan errors but SUSI logged an error
        if (result.errors.empty() && HighestLevelRecorder::getHighest() >= SUSI_LOG_LEVEL_ERROR)
        {
            LOGERROR("Error logged from SUSI while scanning " << escapedPath);
        }

        return result;
    }

    scan_messages::MetadataRescanResponse UnitScanner::metadataRescan(
        std::string_view threatType,
        std::string_view threatName,
        std::string_view path,
        std::string_view sha256)
    {
        HighestLevelRecorder::reset();

        SusiScanResult* susiScanResult = nullptr;

        long tid = syscall(SYS_gettid);
        std::stringstream paddedThreadId;
        paddedThreadId << std::setfill('0') << std::setw(8);
        paddedThreadId << std::hex << tid << std::dec;

        LOG_SUSI_DEBUG("D T" << paddedThreadId.str() << " Starting metadata rescan");

        nlohmann::json metadata = nlohmann::json::object();
        metadata["rescan"] = nlohmann::json::object();
        metadata["rescan"]["sha256"] = sha256;
        metadata["rescan"]["threatName"] = threatName;
        metadata["rescan"]["threatType"] = threatType;
        metadata["rescan"]["path"] = path;

        const auto metadataString = metadata.dump();
        SusiResult res = susi_->metadataRescan(metadataString.c_str(), &susiScanResult);

        LOG_SUSI_DEBUG("D T" << paddedThreadId.str() << " Finished metadata rescan");

        susi_->freeResult(susiScanResult);

        scan_messages::MetadataRescanResponse result;
        if (res == SUSI_S_OK)
        {
            result = scan_messages::MetadataRescanResponse::undetected;
        }
        else if (res == SUSI_I_CLEAN)
        {
            result = scan_messages::MetadataRescanResponse::clean;
        }
        else if (res == SUSI_I_THREATPRESENT)
        {
            result = scan_messages::MetadataRescanResponse::threatPresent;
        }
        else if (res == SUSI_I_NEEDSFULLSCAN)
        {
            result = scan_messages::MetadataRescanResponse::needsFullScan;
        }
        else
        {
            if (!SUSI_FAILURE(res))
            {
                LOGWARN("Unknown metadata rescan result: " << std::hex << res << std::dec);
            }
            result = scan_messages::MetadataRescanResponse::failed;
        }

        return result;
    }
} // namespace threat_scanner