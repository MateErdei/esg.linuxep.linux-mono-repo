// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanCallbackImpl.h"
#include "TimeDuration.h"

#include "Logger.h"

#include "common/StringUtils.h"

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

void ScanCallbackImpl::cleanFile(const path&)
{
    incrementCleanFileCount();
}

void ScanCallbackImpl::infectedFile(const std::map<path, std::string>& detections, const fs::path& realPath, const std::string& scanType, bool isSymlink)
{
    incrementInfectedFileCount();

    for (const auto& detection: detections)
    {
        std::string escapedPath(common::escapePathForLogging(detection.first));
        std::string threatName = detection.second;

        if (isSymlink)
        {
            std::string escapedTargetPath(common::escapePathForLogging(fs::canonical(realPath)));
            LOGWARN(
                "Detected \"" << escapedPath << "\" (symlinked to " << escapedTargetPath << ") is infected with "
                              << threatName << " (" << scanType << ")");
        }
        else
        {
            LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName << " (" << scanType << ")");
        }

        addThreat(threatName);
    }
}

void ScanCallbackImpl::scanError(const std::string& errorMsg, std::error_code errorCode)
{
    incrementErrorCount();
    if (errorCode.category() == std::system_category() && errorCode.value() == ENOENT)
    {
        LOGINFO(errorMsg);
    }
    else
    {
        LOGERROR(errorMsg);
    }
}

common::E_ERROR_CODES ScanCallbackImpl::returnCode()
{
    if (getNoOfInfectedFiles() > 0)
    {
        return common::E_VIRUS_FOUND;
    }
    else if (m_returnCode == common::E_PASSWORD_PROTECTED)
    {
        return common::E_PASSWORD_PROTECTED;
    }
    else if (getNoOfScanErrors() > 0)
    {
        return common::E_GENERIC_FAILURE;
    }
    else
    {
        return common::E_CLEAN_SUCCESS;
    }
}

void ScanCallbackImpl::logSummary()
{
    auto endTime = time(nullptr);
    auto totalScanTime = static_cast<double>(endTime - getStartTime());

    TimeDuration convertedTime(totalScanTime);

    LOGINFO("End of Scan Summary:");

    LOGINFO(getNoOfScannedFiles() << common::pluralize(getNoOfScannedFiles(), " file", " files") << " scanned in " << convertedTime.toString());

    std::ostringstream scanSummary;
    scanSummary << getNoOfInfectedFiles() << common::pluralize(getNoOfInfectedFiles(), " file", " files") << " out of ";
    scanSummary << getNoOfScannedFiles() << common::pluralize(getNoOfInfectedFiles(), " was", " were") << " infected.";
    LOGINFO(scanSummary.str());

    if (getNoOfScanErrors() > 0)
    {
        LOGINFO(getNoOfScanErrors() << " scan" << common::pluralize(getNoOfScanErrors(), " error", " errors") << " encountered.");
    }

    for (const auto& threatType : getThreatTypes())
    {
        LOGINFO(threatType.second << " " << threatType.first << common::pluralize(threatType.second, " infection", " infections") << " discovered.");
    }
}