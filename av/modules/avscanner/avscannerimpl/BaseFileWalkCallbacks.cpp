/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseFileWalkCallbacks.h"

#include "Logger.h"

#include <common/AbortScanException.h>
#include <common/StringUtils.h>

using namespace avscanner::avscannerimpl;

BaseFileWalkCallbacks::BaseFileWalkCallbacks(std::shared_ptr<IScanClient> scanner) : m_scanner(std::move(scanner)) {}

bool BaseFileWalkCallbacks::excludeSymlink(const fs::path& path)
{
    fs::path targetPath = fs::canonical(path);
    const std::string targetPathWithSlash = common::PathUtils::appendForwardSlashToPath(targetPath);
    std::string escapedTarget = common::escapePathForLogging(targetPath);

    for (const auto& e : m_mountExclusions)
    {
        if (common::PathUtils::startswith(targetPathWithSlash, e))
        {
            LOGINFO(
                "Skipping the scanning of symlink target (\""
                << escapedTarget << "\") which is on excluded mount point: " << common::escapePathForLogging(e));
            return true;
        }
    }

    for (const auto& exclusion : m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(path))
        {
            LOGINFO("Excluding symlinked file: " << common::escapePathForLogging(path));
            return true;
        }

        if (exclusion.appliesToPath(targetPath))
        {
            LOGINFO(
                "Skipping the scanning of symlink target (\"" << escapedTarget
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
        if (excludeSymlink(path))
        {
            return;
        }
    }
    else
    {
        for (const auto& exclusion : m_userDefinedExclusions)
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
        m_scanner->scan(path, symlinkTarget);
    }
    catch (const std::exception& e)
    {
        genericFailure(e, escapedPath);
    }
}

bool BaseFileWalkCallbacks::includeDirectory(const sophos_filesystem::path& path)
{
    const std::string pathWithSlash = common::PathUtils::appendForwardSlashToPath(path);

    for (const auto& exclusion : m_currentExclusions)
    {
        if (exclusion.appliesToPath(pathWithSlash, true))
        {
            return false;
        }
    }

    // if we don't remove the forward slash, it will be resolved to the target path
    if (fs::is_symlink(fs::symlink_status(common::PathUtils::removeForwardSlashFromPath(path))))
    {
        // fs canonical resolves the symlink and creates the absolute path to the target
        fs::path targetPath = fs::canonical(path);
        const std::string targetPathWithSlash = common::PathUtils::appendForwardSlashToPath(targetPath);

        for (const auto& exclusion : m_currentExclusions)
        {
            if (exclusion.appliesToPath(targetPathWithSlash, true))
            {
                return false;
            }
        }

        LOGDEBUG("Checking exclusions against symlink directory: " << common::escapePathForLogging(path));
        if (userDefinedExclusionCheck(targetPath, true))
        {
            return false;
        }
    }

    if (userDefinedExclusionCheck(path, false))
    {
        return false;
    }

    return true;
}

bool BaseFileWalkCallbacks::userDefinedExclusionCheck(const sophos_filesystem::path& path, bool isSymlink)
{
    // N.B. doesn't check targetPath too, need to call this twice for symlinks (with and without isSymlink set)
    const std::string pathWithSlash = common::PathUtils::appendForwardSlashToPath(path);

    for (const auto& exclusion : m_userDefinedExclusions)
    {
        if (exclusion.appliesToPath(pathWithSlash, true))
        {
            if (isSymlink)
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

    m_scanner->scanError(errorString);
    m_returnCode = E_GENERIC_FAILURE;
    throw AbortScanException(e.what());
}
