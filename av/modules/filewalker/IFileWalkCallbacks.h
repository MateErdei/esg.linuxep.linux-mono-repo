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
         * File walker process a path.
         * @param filepath
         */
        virtual void processFile(const sophos_filesystem::path& filepath) = 0;
    protected:
        IFileWalkCallbacks() = default;
    };
}
