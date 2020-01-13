/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include <utility>
#include <iostream>

using namespace filewalker;

FileWalker::FileWalker(fs::path p)
    : m_starting_path(std::move(p))
{
}

void FileWalker::run()
{
    for(const auto& p: fs::recursive_directory_iterator(m_starting_path))
    {
        std::cout << p.path() << std::endl;
    }
}
