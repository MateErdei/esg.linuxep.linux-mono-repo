/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystemReplaceAndRestore.h"
#include <Common/FileSystemImpl/FileSystemImpl.h>

void Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> pointerToReplace)
{
    Common::FileSystem::fileSystemStaticPointer() = std::move(pointerToReplace);
}

void Tests::restoreFileSystem()
{
    Common::FileSystem::fileSystemStaticPointer().reset( new Common::FileSystem::FileSystemImpl());
}
