/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FilePermissionsReplaceAndRestore.h"

#include <Common/FileSystemImpl/FilePermissionsImpl.h>

void Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions> pointerToReplace)
{
    Common::FileSystem::filePermissionsStaticPointer() = std::move(pointerToReplace);
}

void Tests::restoreFilePermissions()
{
    Common::FileSystem::filePermissionsStaticPointer().reset(new Common::FileSystem::FilePermissionsImpl());
}