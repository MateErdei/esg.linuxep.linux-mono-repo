/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseFileWalkCallbacks.h"

#include "Logger.h"

#include <common/AbortScanException.h>
#include <common/StringUtils.h>

using namespace avscanner::avscannerimpl;

BaseFileWalkCallbacks::BaseFileWalkCallbacks(ScanClient scanner)
        : m_scanner(std::move(scanner))
{
}

bool BaseFileWalkCallbacks::processSymlinkExclusions(const fs::path& path)
{
    fs::path symlinkTargetPath = fs::canonical(path);
    std::string escapedTarget = common::escapePathForLogging(symlinkTargetPath);

    for (const auto& e : m_mountExclusions)
    {
        if (common::PathUtils::startswith(symlinkTargetPath, e))
        {
            LOGINFO("Skipping the scanning of symlink target (\"" << escapedTarget << "\") which is on excluded mount point: " << common::escapePathForLogging(e));
            return true;
        }
    }

    for (const auto& exclusion: m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(path))
        {
            LOGINFO("Excluding symlinked file: " << common::escapePathForLogging(path));
            return true;
        }

        if (exclusion.appliesToPath(symlinkTargetPath))
        {
            LOGINFO("Skipping the scanning of symlink target (\"" << escapedTarget
                                                                  << "\") which is excluded by user defined exclusion: "
                                                                  << common::escapePathForLogging(exclusion.path()));
            return true;
        }
    }

    return false;
}

void BaseFileWalkCallbacks::processFile(const fs::path& path, bool symlinkTarget)
{
    std::string escapedPath(common::escapePathForLogging(path));
    if (symlinkTarget)
    {
        if(processSymlinkExclusions(path))
        {
            return;
        }
    }
    else
    {
        for (const auto& exclusion: m_userDefinedExclusions)
        {
            if (exclusion.appliesToPath(path))
            {
                LOGINFO("Excluding file: " << escapedPath);
                return;
            }
        }
    }

    logScanningLine(escapedPath);

    try
    {
        m_scanner.scan(path, symlinkTarget);
    }
    catch (const std::exception& e)
    {
        genericFailure(e, escapedPath);
    }
}

bool BaseFileWalkCallbacks::includeDirectory(const sophos_filesystem::path& path)
{
    for (const auto& exclusion: m_currentExclusions)
    {
        if (common::PathUtils::startswith(path, exclusion.path()))
        {
            return false;
        }
    }

    // if we don't remove the forward slash, it will be resolved to the target path
    if (fs::is_symlink(fs::symlink_status(common::PathUtils::removeForwardSlashFromPath(path))))
    {
        // fs canonical resolves the symlink and creates the absolute path to the target
        fs::path symlinkTargetPath = fs::canonical(path);

        for (const auto& exclusion: m_currentExclusions)
        {
            if (common::PathUtils::startswith(symlinkTargetPath, exclusion.path()))
            {
                return false;
            }
        }

        LOGDEBUG("Checking exclusions against symlink directory: " << common::escapePathForLogging(path));
        return !userDefinedExclusionCheck(symlinkTargetPath, true);
    }

    return !userDefinedExclusionCheck(path, false);
}

bool BaseFileWalkCallbacks::userDefinedExclusionCheck(const sophos_filesystem::path& path, bool isSymlink)
{
    const std::string pathWithSlash = common::PathUtils::appendForwardSlashToPath(path);

    for (const auto& exclusion: m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(pathWithSlash, true))
        {
            if(isSymlink)
            {
                LOGINFO("Skipping the scanning of symlink target (\"" << common::escapePathForLogging(fs::canonical(path))
                                                                      << "\") which is excluded by user defined exclusion: "
                                                                      <<  common::escapePathForLogging(exclusion.path()));
            }
            else
            {
                LOGINFO("Excluding directory: " << common::escapePathForLogging(pathWithSlash));
            }
            return true;
        }
    }

    return false;
}

void BaseFileWalkCallbacks::genericFailure(const std::exception& e, const std::string& escapedPath)
{
    std::ostringstream errorString;
    errorString << "Failed to scan" << escapedPath << " [" << e.what() << "]";

    m_scanner.scanError(errorString);
    m_returnCode = E_GENERIC_FAILURE;
    throw AbortScanException(e.what());
}
