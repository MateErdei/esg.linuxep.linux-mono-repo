/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IFileSystem.h"

namespace Common::FileSystem
{
    class ITempDir
    {
    public:
        virtual ~ITempDir() {}
        virtual Path dirPath() const = 0;
    };
} // namespace Common::FileSystem
