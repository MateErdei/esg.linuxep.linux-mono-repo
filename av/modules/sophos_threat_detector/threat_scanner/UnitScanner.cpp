// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "UnitScanner.h"

#include "Logger.h"
#include "SusiLogger.h"
#include "SusiResultUtils.h"
#include "SusiScanResultJsonParser.h"

#include "common/StringUtils.h"

#include <Common/FileSystem/IFileSystem.h>
#include <sys/syscall.h>

#include <iomanip>
#include <unistd.h>

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
            result.errors.push_back(
                { "Error logged from SUSI while scanning " + escapedPath, log4cplus::ERROR_LOG_LEVEL });
        }

        return result;
    }
} // namespace threat_scanner