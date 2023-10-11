/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_filesystem.h"

namespace filewalker
{
    class IFileWalkCallbacks
    {
    public:
        /**
         * Callback for regular files.
         *
         * @param filepath
         */
        virtual void processFile(const sophos_filesystem::path& filepath) = 0;

        /**
         * Callback for new directories - should we recurse into them?
         *
         * This is for implementing exclusions.
         * @param filepath
         * @return True if we should recurse into this directory
         */
        virtual bool includeDirectory(const sophos_filesystem::path& filepath) = 0;

    protected:
        IFileWalkCallbacks() = default;
    };
}
