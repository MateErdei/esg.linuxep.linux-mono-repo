/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include "Logger.h"

#include <common/AbortScanException.h>
#include <common/PathUtils.h>
#include <common/StringUtils.h>

#include <cstring>

#include <sys/stat.h>

namespace fs = sophos_filesystem;

using namespace filewalker;

void FileWalker::walk(const sophos_filesystem::path& starting_point)
{
    if(starting_point.string().size() > 4096)
    {
        std::string errorMsg = "Failed to start scan: Starting Path too long";
        LOGERROR(errorMsg);
        std::error_code ec (ENAMETOOLONG, std::system_category());
        throw fs::filesystem_error(errorMsg, ec);
    }

    fs::file_status itemStatus;
    fs::file_status symlinkStatus;
    try
    {
        itemStatus = fs::status(starting_point);
        // if we don't remove the forward slash, it will be resolved to the target path
        symlinkStatus = fs::symlink_status(common::PathUtils::removeForwardSlashFromPath(starting_point));
    }
    catch (const fs::filesystem_error& e)
    {
        std::ostringstream oss;
        oss << "Failed to scan \"" << common::escapePathForLogging(starting_point) << "\": " << e.code().message();
        LOGERROR(oss.str());
        throw fs::filesystem_error(oss.str(), e.code());        // Backtrack protection for symlinks

    }

    if (!fs::exists(itemStatus))
    {
        std::ostringstream oss;
        oss << "Failed to scan \"" << common::escapePathForLogging(starting_point) << "\": file/folder does not exist";
        LOGERROR(oss.str());
        std::error_code ec (ENOENT, std::system_category());
        throw fs::filesystem_error(oss.str(), ec);
    }

    m_startIsSymlink = fs::is_symlink(symlinkStatus);

    // if starting point is a file either skip or scan it
    // else starting point is a directory either skip or continue to traversal
    if (fs::is_regular_file(itemStatus))
    {
        try
        {
            m_callback.processFile(starting_point, m_startIsSymlink);
        }
        catch (const AbortScanException&)
        {
            throw;
        }
        catch (const std::runtime_error& ex)
        {
            std::ostringstream stringError;
            stringError << "Failed to process: " << common::escapePathForLogging(starting_point);
            m_callback.registerError(stringError);
        }
        return;
    }
    else if (fs::is_directory(itemStatus))
    {
        // TODO - unnecessary, but unit tests currently expect it
        if (m_callback.userDefinedExclusionCheck(starting_point, m_startIsSymlink))
        {
            return;
        }
    }
    else
    {
        LOGINFO("Not scanning special file/device: \"" << common::escapePathForLogging(starting_point) << "\"");
        return;
    }

    m_options = fs::directory_options::skip_permission_denied;

    if (m_follow_symlinks)
    {
        m_options |= fs::directory_options::follow_directory_symlink;
    }

    if (m_stay_on_device)
    {
        struct stat statBuf {};
        int ret = ::stat(starting_point.c_str(), &statBuf);
        if (ret != 0)
        {
            int error_number = errno;
            std::ostringstream oss;
            oss << "Failed to stat \"" << common::escapePathForLogging(starting_point) << "\": " << std::strerror(error_number);
            LOGERROR(oss.str());
            std::error_code ec(error_number, std::system_category());
            throw fs::filesystem_error(oss.str(), ec);
        }
        m_starting_dev = statBuf.st_dev;
    }

    m_loggedExclusionCheckFailed = false;
    scanDirectory(starting_point);
}

void FileWalker::scanDirectory(const fs::path& current_dir)
{
    try
    {
        if (!m_callback.includeDirectory(current_dir))
        {
            LOGDEBUG("Not recursing into \"" << common::escapePathForLogging(current_dir) << "\" as it is excluded");
            return;
        }
    }
    catch (const std::runtime_error& e)
    {
        if(!m_loggedExclusionCheckFailed)
        {
            std::ostringstream stringError;
            stringError << "Failed to check exclusions against: " << common::escapePathForLogging(current_dir) << " due to an error: " << e.what();
            m_callback.registerError(stringError);
            m_loggedExclusionCheckFailed = true;
        }
    }

    struct stat statBuf {};
    int ret = ::stat(current_dir.c_str(), &statBuf);
    if (ret != 0)
    {
        std::ostringstream stringError;
        stringError << "Failed to stat " << common::escapePathForLogging(current_dir) << "(" << errno << ")";
        m_callback.registerError(stringError);
        return;
    }

    if (m_stay_on_device)
    {
        if (statBuf.st_dev != m_starting_dev)
        {
            LOGDEBUG("Not recursing into " << common::escapePathForLogging(current_dir) << " as it is on a different mount");
            return;
        }
    }

    // do backtrack protection last, to avoid marking an excluded/skipped directory as visited
    file_id id = std::make_tuple(statBuf.st_dev, statBuf.st_ino);
    if (m_seen_directories.find(id) != m_seen_directories.end())
    {
        LOGDEBUG("Directory already scanned: \"" << common::escapePathForLogging(current_dir) << "\" [" << statBuf.st_dev << ", " << statBuf.st_ino << "]");
        return;
    }
    else
    {
        m_seen_directories.insert(id);
    }

    std::error_code ec {};
    for( auto iterator = fs::directory_iterator(current_dir, m_options, ec);
         iterator != fs::directory_iterator();
         iterator.increment(ec))
    {
        // if the iterator fails, it will set ec and return an end iterator. Check for ec is outside of the loop.
        const auto& p = *iterator;
        fs::file_status symlinkStatus;
        try
        {
            symlinkStatus = p.symlink_status();
        }
        catch (const fs::filesystem_error& e)
        {
            std::ostringstream stringError;
            stringError << "Failed to get the symlink status of: " << common::escapePathForLogging(p.path()) << " [" << e.code().message() << "]";
            m_callback.registerError(stringError);
            continue;
        }

        if (fs::is_symlink(symlinkStatus))
        {
            if (!m_follow_symlinks)
            {
                std::ostringstream stringError;
                stringError << "Not following symlink: " << common::escapePathForLogging(p.path());
                m_callback.registerError(stringError);
                continue;
            }
        }

        fs::file_status itemStatus;
        try
        {
            itemStatus = p.status();
        }
        catch (const fs::filesystem_error& e)
        {
            std::ostringstream stringError;
            stringError << "Failed to get the status of: " << p << " [" << e.code().message() << "]";
            m_callback.registerError(stringError);
            continue;
        }

        if (fs::is_regular_file(itemStatus))
        {
            try
            {
                m_callback.processFile(p.path(), m_startIsSymlink || fs::is_symlink(symlinkStatus));
            }
            catch (const AbortScanException&)
            {
                throw;
            }
            catch (const std::runtime_error& ex)
            {
                std::ostringstream stringError;
                stringError << "Failed to process: " << p.path().string();
                m_callback.registerError(stringError);
                continue;
            }
        }
        else if (fs::is_directory(itemStatus))
        {
            scanDirectory(p);
        }
        else
        {
            // ignoring p
        }
    }
    if (ec)
    {
        std::ostringstream stringError;
        stringError << "Failed to iterate: " << current_dir << ": " << ec.message();
        m_callback.registerError(stringError);
        return;
    }
}