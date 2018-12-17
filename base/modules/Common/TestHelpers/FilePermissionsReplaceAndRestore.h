/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFilePermissions.h"

namespace Common
{
    namespace TestHelpers
    {
        void replaceFilePermissions(Common::FileSystem::IFilePermissionsPtr);
        void restoreFilePermissions();
    }

}