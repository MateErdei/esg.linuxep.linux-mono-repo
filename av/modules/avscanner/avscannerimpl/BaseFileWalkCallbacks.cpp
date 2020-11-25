/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <common/StringUtils.h>
#include "Logger.h"
#include "BaseFileWalkCallbacks.h"

using namespace avscanner::avscannerimpl;

BaseFileWalkCallbacks::BaseFileWalkCallbacks(ScanClient scanner)
        : m_scanner(std::move(scanner))
{
}

bool BaseFileWalkCallbacks::processSymlinkExclusions(const fs::path& path)
{
    fs::path symlinkTargetPath = path;
    if (fs::is_symlink(fs::symlink_status(path)))
    {
        // fs canonical resolves the symlink and creates the absolute path to the target
        symlinkTargetPath = fs::canonical(path);

        LOGINFO("Path: " << path);
        LOGINFO("symlinkTargetPath: " << symlinkTargetPath);
    }

    for (const auto& e : m_mountExclusions)
    {
        if (common::PathUtils::startswith(symlinkTargetPath, e))
        {
            LOGINFO("Skipping the scanning of symlink target (" << symlinkTargetPath << ") which is on excluded mount point: " << e);
            return true;
        }
    }

    for (const auto& exclusion: m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(path))
        {
            LOGINFO("Excluding symlinked file: " << path);
            return true;
        }

        if (exclusion.appliesToPath(symlinkTargetPath))
        {
            LOGINFO("Skipping the scanning of symlink target (" << symlinkTargetPath << ") which is excluded by user defined exclusion: " << exclusion.path());
            return true;
        }
    }

    return false;
}

void BaseFileWalkCallbacks::processFile(const fs::path& path, bool symlinkTarget)
{
    if (symlinkTarget)
    {
        if(processSymlinkExclusions(path))
        {
            return;
        }
    }

    std::string escapedPath(common::toUtf8(path, false, false));
    common::escapeControlCharacters(escapedPath);

    for (const auto& exclusion: m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(path))
        {
            LOGINFO("Excluding file: " << escapedPath);
            return;
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

        LOGINFO("Checking exclusions against symlink directory: " << path);
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
                LOGINFO("Skipping the scanning of symlink target (" << path << ") which is excluded by user defined exclusion: " << exclusion.path());
            }
            else
            {
                LOGINFO("Excluding directory: " << pathWithSlash);
            }
            return true;
        }
    }

    return false;
}