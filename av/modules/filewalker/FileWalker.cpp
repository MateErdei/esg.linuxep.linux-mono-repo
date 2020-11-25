/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include "Logger.h"

#include <common/PathUtils.h>
#include <common/AbortScanException.h>

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
        symlinkStatus = fs::symlink_status(common::PathUtils::removeForwardSlashFromPath(starting_point));
    }
    catch (const fs::filesystem_error& e)
    {
        std::ostringstream oss;
        oss << "Failed to scan " << starting_point << ": " << e.code().message();
        LOGERROR(oss.str());
        throw fs::filesystem_error(oss.str(), e.code());        // Backtrack protection for symlinks

    }

    if (!fs::exists(itemStatus))
    {
        std::ostringstream oss;
        oss << "Failed to scan " << starting_point << ": file/folder does not exist";
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
        catch (const std::runtime_error& ex)
        {
            LOGERROR("Failed to process: " << starting_point.string());
        }
        return;
    }
    else if (fs::is_directory(itemStatus))
    {
        // TODO - superfluous - but need to update all test expectations first?
        if (m_callback.userDefinedExclusionCheck(starting_point, false))
        {
            return;
        }
    }
    else
    {
        LOGINFO("Not scanning special file/device: " << starting_point);
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
            oss << "Failed to stat " << starting_point << ": " << std::strerror(error_number);
            LOGERROR(oss.str());
            std::error_code ec(error_number, std::system_category());
            throw fs::filesystem_error(oss.str(), ec);
        }
        m_starting_dev = statBuf.st_dev;
    }

    scanDirectory(starting_point);
}

void FileWalker::scanDirectory(const fs::path& current_dir)
{
    if (!m_callback.includeDirectory(current_dir))
    {
        LOGDEBUG("Not recursing into " << current_dir << " as it is excluded");
        return;
    }

    struct stat statBuf {};
    int ret = ::stat(current_dir.c_str(), &statBuf);
    if (ret != 0)
    {
        LOGERROR("Failed to stat " << current_dir << "(" << errno << ")");
        return;
    }

    if (m_stay_on_device)
    {
        if (statBuf.st_dev != m_starting_dev)
        {
            LOGDEBUG("Not recursing into " << current_dir << " as it is on a different mount");
            return;
        }
    }

    // do backtrack protection last, to avoid marking an excluded/skipped directory as visited
    file_id id = std::make_tuple(statBuf.st_dev, statBuf.st_ino);
    if (m_seen_symlinks.find(id) != m_seen_symlinks.end())
    {
        LOGDEBUG("Directory already scanned: " << current_dir << " [" << statBuf.st_dev << ", " << statBuf.st_ino << "]");
        return;
    }
    else
    {
        m_seen_symlinks.insert(id);
    }

    std::error_code ec{};
    for( auto iterator = fs::directory_iterator(current_dir, m_options, ec);
         iterator != fs::directory_iterator();
         iterator.increment(ec))
    {
        if (ec)
        {
            LOGERROR("Failed to iterate: " << current_dir << ": " << ec.message());
            return;
        }

        const auto& p = *iterator;
        fs::file_status itemStatus;
        fs::file_status symlinkStatus;
        try
        {
            itemStatus = p.status();
            symlinkStatus = p.symlink_status();
        }
        catch (const fs::filesystem_error& e)
        {
            LOGERROR("Failed to get the status of: " << p << " [" << e.code().message() << "]");
            continue;
        }

        // TODO - consider putting this under if (fs::is_directory(...))
        if (fs::is_symlink(symlinkStatus))
        {
            if (!m_follow_symlinks)
            {
                LOGDEBUG("Not following symlink: " << p);
                continue;
            }

            // TODO - fs::read_symlink and exclusion check here?
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
                LOGERROR("Failed to process: " << p.path().string());
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
}

void filewalker::walk(const sophos_filesystem::path& starting_point, IFileWalkCallbacks& callbacks)
{
    FileWalker f(callbacks);
    f.walk(starting_point);
}
