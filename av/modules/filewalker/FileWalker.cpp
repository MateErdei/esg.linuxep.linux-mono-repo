/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"
#include "Logger.h"

#include <set>
#include <sys/stat.h>

namespace fs = sophos_filesystem;

using namespace filewalker;

void FileWalker::walk(const sophos_filesystem::path& starting_point)
{
    if(starting_point.string().size() > 4096)
    {
        std::string errorMsg = "Failed to scan starting Path too long";
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

    bool startIsSymlink = fs::is_symlink(starting_point);

    // if starting point is a file either skip or scan it
    // else starting point is a directory either skip or continue to traversal
    if (fs::is_regular_file(starting_point))
    {
        m_callback.processFile(starting_point, startIsSymlink);
        return;
    }
    else if (fs::is_directory(starting_point))
    {
        if (m_callback.cmdExclusionCheck(starting_point))
        {
            return;
        }
    }

    std::set<ino_t> seen_symlinks;
    fs::directory_options options = fs::directory_options::skip_permission_denied;

    if (m_follow_symlinks)
    {
        options |= fs::directory_options::follow_directory_symlink;
    }

    for(
        auto iterator = fs::recursive_directory_iterator(starting_point, options);
        iterator != fs::recursive_directory_iterator();
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
            iterator.disable_recursion_pending();
            continue;
        }

        if (isRegularFile)
        {
            // Regular file
            m_callback.processFile(p, startIsSymlink || fs::is_symlink(p.symlink_status()));
        }
        else if (fs::is_symlink(p.symlink_status()))
        {
            // Directory symlink
            struct stat statbuf{};
            int ret = ::lstat(p.path().c_str(), &statbuf);
            if (ret == 0)
            {
                if (seen_symlinks.find(statbuf.st_ino) != seen_symlinks.end())
                {
                    LOGDEBUG("Disable recursion " << p << " " << statbuf.st_ino);
                    iterator.disable_recursion_pending();
                }
                else
                {
                    seen_symlinks.insert(statbuf.st_ino);
                    if (!m_callback.includeDirectory(p))
                    {
                        iterator.disable_recursion_pending();
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
            if (!m_callback.includeDirectory(p))
            {
                iterator.disable_recursion_pending();
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
