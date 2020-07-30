/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FilePermissionsReplaceAndRestore.h"

#include <Common/FileSystemImpl/FilePermissionsImpl.h>
namespace{
    std::mutex preventTestsToUseEachOtherFileSystemMock; 
}

void Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions> pointerToReplace)
{
    Common::FileSystem::filePermissionsStaticPointer() = std::move(pointerToReplace);
}

void Tests::restoreFilePermissions()
{
    Common::FileSystem::filePermissionsStaticPointer().reset(new Common::FileSystem::FilePermissionsImpl());
}

namespace Tests{
        
    ScopedReplaceFilePermissions::ScopedReplaceFilePermissions() : m_guard{preventTestsToUseEachOtherFileSystemMock}{
        restoreFilePermissions(); 
    }

    ScopedReplaceFilePermissions::ScopedReplaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions> pointerToReplace) : ScopedReplaceFilePermissions()
    {        
        replace(std::move(pointerToReplace)); 
    }

    ScopedReplaceFilePermissions::ScopedReplaceFilePermissions(UseNullFilePermission) : ScopedReplaceFilePermissions( std::unique_ptr<Common::FileSystem::IFilePermissions>(new NullFilePermission())){
    }

    void ScopedReplaceFilePermissions::replace(std::unique_ptr<Common::FileSystem::IFilePermissions> pointerToReplace)
    {
        replaceFilePermissions(std::move(pointerToReplace)); 
    }

    ScopedReplaceFilePermissions::~ScopedReplaceFilePermissions()
    {
        restoreFilePermissions(); 
    }
}