/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_filesystem.h"

#include <functional>

namespace fs = sophos_filesystem;

namespace filewalker
{
    class FileWalker
    {
    public:
        using callback_t = std::function<void (const fs::path&)>;
        FileWalker(fs::path starting_point, callback_t func);

        void run();
    private:
        const fs::path m_starting_path;
        callback_t m_callback;
    };
}
