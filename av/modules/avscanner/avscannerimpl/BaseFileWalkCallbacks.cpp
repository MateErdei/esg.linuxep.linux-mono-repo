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

void BaseFileWalkCallbacks::processFile(const fs::path& path, bool symlinkTarget)
{
    if (symlinkTarget)
    {
        fs::path symlinkTargetPath = path;
        if (fs::is_symlink(fs::symlink_status(path)))
        {
            symlinkTargetPath = fs::read_symlink(path);
        }
        for (const auto& e : m_mountExclusions)
        {
            if (PathUtils::startswith(symlinkTargetPath, e))
            {
                LOGINFO("Skipping the scanning of symlink target (" << symlinkTargetPath << ") which is on excluded mount point: " << e);
                return;
            }
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
        if (PathUtils::startswith(path, exclusion.path()))
        {
            return false;
        }
    }
    return !userDefinedExclusionCheck(path);
}

bool BaseFileWalkCallbacks::userDefinedExclusionCheck(const sophos_filesystem::path& path)
{
    const std::string pathWithSlash = PathUtils::appendForwardSlashToPath(path);

    for (const auto& exclusion: m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(pathWithSlash, true))
        {
            LOGINFO("Excluding directory: " << pathWithSlash);
            return true;
        }
    }

    return false;
}