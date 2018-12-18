/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/FileSystem/IFileSystem.h"

namespace Tests
{
    void replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>);
    void restoreFileSystem();
}
