/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

namespace Common
{
    namespace FileSystem
    {
        class ITempDir
        {
        public:
            virtual ~ITempDir() {}
            virtual Path dirPath() const = 0;
        };
    }
}
