/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include "datatypes/Print.h"

#include <utility>

namespace fs = sophos_filesystem;

using namespace filewalker;

void FileWalker::walk(const sophos_filesystem::path& starting_point)
{
    fs::directory_options options = fs::directory_options::none;
    if (m_follow_symlinks)
    {
        options |= fs::directory_options::follow_directory_symlink;
    }
    for(const auto& p: fs::recursive_directory_iterator(starting_point, options))
    {
        if (fs::is_regular_file(p.status()))
        {
            m_callback.processFile(p);
        }
        else
        {
            PRINT("Not calling with " << p);
        }
    }
}

void filewalker::walk(const sophos_filesystem::path& starting_point, IFileWalkCallbacks& callbacks)
{
    FileWalker f(callbacks);
    f.walk(starting_point);
}
