/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include <utility>

namespace fs = sophos_filesystem;

using namespace filewalker;

FileWalker::FileWalker(fs::path starting_point, IFileWalkCallbacks& func)
    : m_starting_path(std::move(starting_point)), m_callback(func)
{
}


void FileWalker::run()
{
    for(const auto& p: fs::recursive_directory_iterator(m_starting_path))
    {
        if (fs::is_regular_file(p.status()))
        {
            m_callback.processFile(p);
        }
    }
}

void filewalker::walk(sophos_filesystem::path starting_point, IFileWalkCallbacks& callbacks)
{
    FileWalker f(std::move(starting_point), callbacks);
    f.run();
}
