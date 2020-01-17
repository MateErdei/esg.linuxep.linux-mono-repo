/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include <utility>

using namespace filewalker;

FileWalker::FileWalker(fs::path starting_point, callback_t func)
    : m_starting_path(std::move(starting_point)), m_callback(std::move(func))
{
}


void FileWalker::run()
{
    for(const auto& p: fs::recursive_directory_iterator(m_starting_path))
    {
        m_callback(p);
    }
}