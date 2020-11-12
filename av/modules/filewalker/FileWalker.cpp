/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include "Logger.h"

#include <cstring>

#include <sys/stat.h>

namespace fs = sophos_filesystem;

using namespace filewalker;

void FileWalker::walk(const sophos_filesystem::path& starting_point)
{
    m_seen_symlinks.clear();
    m_starting_dev = 0;

    if(starting_point.string().size() > 4096)
    {
        std::string errorMsg = "Failed to start scan: Starting Path too long";
        LOGERROR(errorMsg);
        std::error_code ec (ENAMETOOLONG, std::system_category());
        throw fs::filesystem_error(errorMsg, ec);
    }

    bool fileExists;
    try
    {
        fileExists = fs::exists(starting_point);
    }
    catch(fs::filesystem_error& e)
    {
        std::ostringstream oss;
        oss << "Failed to scan " << starting_point << ": " << e.code().message();
        LOGERROR(oss.str());
        throw fs::filesystem_error(oss.str(), e.code());
    }

    if (!fileExists)
    {
        std::ostringstream oss;
        oss << "Failed to scan " << starting_point << ": file/folder does not exist";
        LOGERROR(oss.str());
        std::error_code ec (ENOENT, std::system_category());
        throw fs::filesystem_error(oss.str(), ec);
    }

    m_startIsSymlink = fs::is_symlink(starting_point);

    // if starting point is a file either skip or scan it
    // else starting point is a directory either skip or continue to traversal
    if (fs::is_regular_file(starting_point))
    {
        m_callback.processFile(starting_point, m_startIsSymlink);
        return;
    }
    else if (fs::is_directory(starting_point))
    {
        if (m_callback.userDefinedExclusionCheck(starting_point))
        {
            return;
        }
    }

    m_options = fs::directory_options::skip_permission_denied;

    if (m_follow_symlinks)
    {
        m_options |= fs::directory_options::follow_directory_symlink;
    }

    if (m_stay_on_device)
    {
        struct stat statbuf{};
        int ret = ::stat(starting_point.c_str(), &statbuf);
        if (ret != 0)
        {
            int error_number = errno;
            std::ostringstream oss;
            oss << "Failed to stat " << starting_point << ": " << std::strerror(error_number);
            LOGERROR(oss.str());
            std::error_code ec(error_number, std::system_category());
            throw fs::filesystem_error(oss.str(), ec);
        }
        m_starting_dev = statbuf.st_dev;
    }

    scanDirectory(starting_point);
}

void FileWalker::scanDirectory(const fs::path& starting_point)
{
    std::error_code ec{};
    auto iterator = fs::directory_iterator(starting_point, m_options, ec);
    if (ec)
    {
        LOGERROR("Failed to iterate: " << starting_point << ": " << ec.message());
        return;
    }

    for(
        ;
        iterator != fs::directory_iterator();
        ++iterator )
    {
        const auto& p = *iterator;
        bool isRegularFile;
        try
        {
            isRegularFile = fs::is_regular_file(p.status());
        }
        catch(fs::filesystem_error& e)
        {
            LOGERROR("Failed to access " << p << ": " << e.code().message());
            continue;
        }

        if (isRegularFile)
        {
            // Regular file
            try
            {
                m_callback.processFile(p.path(), m_startIsSymlink || fs::is_symlink(p.symlink_status()));
            }
            catch (const std::runtime_error& ex)
            {
                LOGERROR("Failed to process: " << p.path().string());
                continue;
            }
        }
        else if (fs::is_symlink(p.symlink_status()))
        {
            // Directory symlink
            struct stat statbuf{};
            int ret = ::lstat(p.path().c_str(), &statbuf);
            if (ret == 0)
            {
                if (m_seen_symlinks.find(statbuf.st_ino) != m_seen_symlinks.end())
                {
                    LOGDEBUG("Symlink target already scanned: " << p << " " << statbuf.st_ino);
                }
                else
                {
                    m_seen_symlinks.insert(statbuf.st_ino);
                    if (m_callback.includeDirectory(p.path()))
                    {
                        scanDirectory(p);
                    }
                }
            }
            else
            {
                LOGERROR("Failed to lstat "<< p << "("<<errno<<")");
            }
        }
        else if (fs::is_directory(p.status()))
        {
            if (m_callback.includeDirectory(p.path()))
            {
                if (m_stay_on_device)
                {
                    struct stat statbuf{};
                    int ret = ::stat(p.path().c_str(), &statbuf);
                    if (ret == 0)
                    {
                        if (statbuf.st_dev != m_starting_dev)
                        {
                            LOGDEBUG("Not recursing into " << p.path() << " as it is on a different mount");
                            continue;
                        }
                    }
                }
                scanDirectory(p);
            }
            else
            {
                LOGDEBUG("Not recursing into " << p.path() << " as it is excluded");
            }
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
