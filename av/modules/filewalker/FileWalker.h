/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_filesystem.h"
#include "IFileWalkCallbacks.h"

#include <functional>

namespace filewalker
{
    class FileWalker
    {
    public:
        /**
         * Construct a new file walker
         *
         * @param starting_point
         * @param callbacks BORROWED reference to callbacks
         */
        FileWalker(sophos_filesystem::path starting_point, IFileWalkCallbacks& callbacks);

        void run();
    private:
        const sophos_filesystem::path m_starting_path;
        IFileWalkCallbacks& m_callback;
    };

    void walk(sophos_filesystem::path starting_point, IFileWalkCallbacks& callbacks);
}
