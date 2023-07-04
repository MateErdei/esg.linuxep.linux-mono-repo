// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "FilePermissionsReplaceAndRestore.h"

#include "Common/FileSystemImpl/FilePermissionsImpl.h"

#include <mutex>

namespace{
    std::mutex preventTestsToUseEachOtherFilePermissionsMock;
}

void Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions> pointerToReplace)
{
    Common::FileSystem::filePermissionsStaticPointer() = std::move(pointerToReplace);
    pointerToReplace.release();
}

void Tests::restoreFilePermissions()
{
    Common::FileSystem::filePermissionsStaticPointer().reset(new Common::FileSystem::FilePermissionsImpl());
}

namespace Tests{

    ScopedReplaceFilePermissions::ScopedReplaceFilePermissions() : m_guard{preventTestsToUseEachOtherFilePermissionsMock}{
        restoreFilePermissions();
    }

    ScopedReplaceFilePermissions::ScopedReplaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions> pointerToReplace) : ScopedReplaceFilePermissions()
    {
        replace(std::move(pointerToReplace));
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