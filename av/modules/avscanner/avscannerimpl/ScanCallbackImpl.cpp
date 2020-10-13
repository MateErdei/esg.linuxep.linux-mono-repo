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

void ScanCallbackImpl::infectedFile(const path& p, const std::string& threatName, bool isSymlink)
{
    std::string escapedPath(p);
    common::escapeControlCharacters(escapedPath);

    if (isSymlink)
    {
        std::string escapedTargetPath(fs::canonical(p));
        common::escapeControlCharacters(escapedTargetPath);
        LOGWARN("Detected \"" << escapedPath << "\" (symlinked to " << escapedTargetPath << ") is infected with " << threatName);
    }
    else
    {
        LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName);
    }

    incrementInfectedCount();
    m_returnCode = E_VIRUS_FOUND;
}

void ScanCallbackImpl::scanError(const std::string& errorMsg)
{
    LOGERROR(errorMsg);
}

void ScanCallbackImpl::logSummary()
{
    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> totalScanTime = endTime - getStartTime();
    std::ostringstream scanSummary;

    scanSummary << std::endl;
    scanSummary << getNoOfScannedFiles() << " file(s) scanned in " << totalScanTime.count()  << " seconds." << std::endl;
    scanSummary << getNoOfInfectedFiles() << " threats discovered." << std::endl; //TODO count number of different threats?
    scanSummary << getNoOfInfectedFiles() << " file(s) out of " << getNoOfScannedFiles() << " was infected." << std::endl;
    scanSummary << "End of Scan" << std::endl;

    LOGINFO(scanSummary.str());
}