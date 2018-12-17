/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFilePermissions.h"

namespace Common
{
    namespace TestHelpers
    {
        void replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>);
        void restoreFilePermissions();
    }

}