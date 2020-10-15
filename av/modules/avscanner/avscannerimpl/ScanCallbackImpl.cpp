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
    auto totalScanTime = endTime - getStartTime();
    std::ostringstream scanSummary;

    scanSummary << "End of Scan Summary:" << std::endl;
    scanSummary << getNoOfScannedFiles() << " file(s) scanned in " << static_cast<double>(totalScanTime) << " seconds." << std::endl;
    scanSummary << getNoOfInfectedFiles() << " file(s) out of " << getNoOfScannedFiles() << " was infected." << std::endl;

    if (getNoOfScanErrors() > 0)
    {
        scanSummary << getNoOfScanErrors() << " scan Error(s) encountered" << std::endl;
    }
    for (const auto& threatType : getThreatTypes())
    {
        scanSummary << threatType.second << " threat(s) of type " <<  threatType.first << " discovered." << std::endl;
    }

    LOGINFO(scanSummary.str());
}
