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
    incrementCleanCount();
}

void ScanCallbackImpl::infectedFile(const path& path, const std::string& threatName, bool isSymlink)
{
    std::string escapedPath(path);
    common::escapeControlCharacters(escapedPath);

    if (isSymlink)
    {
        std::string escapedTargetPath(fs::canonical(path));
        common::escapeControlCharacters(escapedTargetPath);
        LOGWARN("Detected \"" << escapedPath << "\" (symlinked to " << escapedTargetPath << ") is infected with " << threatName);
    }
    else
    {
        LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName);
    }

    addThreat(threatName);
    incrementInfectedCount();
    m_returnCode = E_VIRUS_FOUND;
}

void ScanCallbackImpl::scanError(const std::string& errorMsg)
{
    incrementErrorCount();
    LOGERROR(errorMsg);
}

void ScanCallbackImpl::logSummary()
{
    auto endTime = time(nullptr);
    auto totalScanTime = static_cast<double>(endTime - getStartTime());
    std::ostringstream scanSummary;

    scanSummary << "End of Scan Summary:" << std::endl;

    scanSummary << getNoOfScannedFiles() << common::pluralize(getNoOfScannedFiles(), " file", " files") << " scanned in ";
    scanSummary << totalScanTime << common::pluralize(totalScanTime, " second.", " seconds.") << std::endl;

    scanSummary << getNoOfInfectedFiles() << common::pluralize(getNoOfInfectedFiles(), " file", " files") << " out of ";
    scanSummary << getNoOfScannedFiles() << common::pluralize(getNoOfScannedFiles(), " was", " were") << " infected." << std::endl;

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
