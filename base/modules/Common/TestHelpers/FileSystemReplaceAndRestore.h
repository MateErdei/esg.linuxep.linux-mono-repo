/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/FileSystem/IFileSystem.h"

namespace Common
{
    namespace TestHelpers
    {
        void replaceFileSystem(Common::FileSystem::IFileSystemPtr);
        void restoreFileSystem();
    }
}
