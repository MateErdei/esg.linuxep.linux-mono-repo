/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystemReplaceAndRestore.h"

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <mutex>
#include<iostream>
namespace{
    std::mutex preventTestsToUseEachOtherFileSystemMock; 
}


void Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> pointerToReplace)
{
    Common::FileSystem::fileSystemStaticPointer().reset(pointerToReplace.get());
    pointerToReplace.release(); 
}

void Tests::restoreFileSystem()
{
    Common::FileSystem::fileSystemStaticPointer().reset(new Common::FileSystem::FileSystemImpl());
}

namespace Tests{
        
    ScopedReplaceFileSystem::ScopedReplaceFileSystem() : m_guard{preventTestsToUseEachOtherFileSystemMock}{
        restoreFileSystem(); 
    }

    ScopedReplaceFileSystem::ScopedReplaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> pointerToReplace) : ScopedReplaceFileSystem()
    {        
        replace(std::move(pointerToReplace)); 
    }

    void ScopedReplaceFileSystem::replace(std::unique_ptr<Common::FileSystem::IFileSystem> pointerToReplace)
    {
        replaceFileSystem(std::move(pointerToReplace)); 
    }

    ScopedReplaceFileSystem::~ScopedReplaceFileSystem()
    {
        restoreFileSystem(); 
    }
}