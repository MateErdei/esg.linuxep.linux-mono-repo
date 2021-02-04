/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanCallbackImpl.h"

#include "Logger.h"

#include "common/StringUtils.h"

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

void ScanCallbackImpl::cleanFile(const path&)
{
    incrementCleanFileCount();
}

void ScanCallbackImpl::infectedFile(const std::map<path, std::string>& detections, const fs::path& realPath, bool isSymlink)
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
                              << threatName);
        }
        else
        {
            LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName);
        }

        addThreat(threatName);
    }
}

void ScanCallbackImpl::scanError(const std::string& errorMsg)
{
    incrementErrorCount();
    LOGERROR(errorMsg);
}

common::E_ERROR_CODES ScanCallbackImpl::returnCode()
{
    if (getNoOfInfectedFiles() > 0)
    {
        return common::E_VIRUS_FOUND;
    }
    else if (getNoOfScanErrors() > 0)
    {
        return common::E_GENERIC_FAILURE;
    }
    else
    {
        return common::E_CLEAN;
    }
}

ScanCallbackImpl::TimeDuration ScanCallbackImpl::timeConversion(const long totalScanTime)
{
    auto newScanTime = int(totalScanTime);

    auto sec = newScanTime % 60;
    newScanTime /= 60;
    auto min = newScanTime % 60;
    newScanTime /= 60;
    auto hour = newScanTime % 24;

    TimeDuration result = {sec,min,hour};

    return result;
}

void ScanCallbackImpl::logSummary()
{
    auto endTime = time(nullptr);
    auto totalScanTime = static_cast<double>(endTime - getStartTime());

    TimeDuration convertedTime = timeConversion(totalScanTime);

    auto hour = convertedTime.hour;
    auto min = convertedTime.min;
    auto sec = convertedTime.sec;

    std::ostringstream scanSummary;

    scanSummary << "End of Scan Summary:" << std::endl;

    scanSummary << getNoOfScannedFiles() << common::pluralize(getNoOfScannedFiles(), " file", " files") << " scanned in ";

    if (hour > 0)
    {
        scanSummary << hour << common::pluralize(hour, " hour, ", " hours, ");
    }
    if (min > 0)
    {
        scanSummary << min << common::pluralize(min, " minute, ", " minutes, ");
    }
    if (hour == 0 && min == 0 && sec == 0)
    {
        scanSummary << "less than a second" << std::endl;
    }
    else
    {
        scanSummary << sec << common::pluralize(sec, " second. ", " seconds. ") << std::endl;
    }

    scanSummary << getNoOfInfectedFiles() << common::pluralize(getNoOfInfectedFiles(), " file", " files") << " out of ";
    scanSummary << getNoOfScannedFiles() << common::pluralize(getNoOfInfectedFiles(), " was", " were") << " infected." << std::endl;

    if (getNoOfScanErrors() > 0)
    {
        scanSummary << getNoOfScanErrors() << " scan" << common::pluralize(getNoOfScanErrors(), " error", " errors") << " encountered." << std::endl;
    }

    for (const auto& threatType : getThreatTypes())
    {
        scanSummary << threatType.second << " " << threatType.first << common::pluralize(threatType.second, " infection", " infections") << " discovered." << std::endl;
    }

    LOGINFO(scanSummary.str());
}
