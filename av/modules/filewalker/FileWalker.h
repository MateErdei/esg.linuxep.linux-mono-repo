/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_filesystem.h"
#include "IFileWalkCallbacks.h"

namespace filewalker
{
    class FileWalker
    {
    public:
        /**
         * Construct a new file walker
         *
         * @param callbacks BORROWED reference to callbacks
         */
        explicit FileWalker(IFileWalkCallbacks& callbacks)
                : m_callback(callbacks)
        {}

        /**
         * Walk a directory, calling callbacks for each file
         * @param starting_point
         */
        void walk(const sophos_filesystem::path& starting_point);
    private:
        IFileWalkCallbacks& m_callback;
    };
    void walk(const sophos_filesystem::path& starting_point, IFileWalkCallbacks& callbacks);
}
