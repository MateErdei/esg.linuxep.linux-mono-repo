//
// Created by pair on 19/12/2019.
//

#pragma once

#include "sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace filewalker
{
    class FileWalker
    {
    public:
        explicit FileWalker(fs::path p);

        void run();
    private:
        const fs::path m_starting_path;
    };
}
