/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"
#include "Logger.h"

#include "datatypes/Print.h"

#include <utility>
#include <set>
#include <sys/stat.h>

namespace fs = sophos_filesystem;

using namespace filewalker;

void FileWalker::walk(const sophos_filesystem::path& starting_point)
{
    std::set<ino_t> seen_symlinks;
    fs::directory_options options = fs::directory_options::none;
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
        if (fs::is_regular_file(p.status()))
        {
            m_callback.processFile(p);
        }
        else if (fs::is_symlink(p.symlink_status()))
        {
//            PRINT("Directory symlink " << p);
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
//                    PRINT("First time " << p << " " << statbuf.st_ino);
                    seen_symlinks.insert(statbuf.st_ino);
                    if (!m_callback.includeDirectory(p))
                    {
                        iterator.disable_recursion_pending();
                    }
                }
            }
            else
            {
                LOGERROR("Failed to stat " << p);
            }
        }
        else if (fs::is_directory(p.status()))
        {
//            PRINT("Not calling with " << p);
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
