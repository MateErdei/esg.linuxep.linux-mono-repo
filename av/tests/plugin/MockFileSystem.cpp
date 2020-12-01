/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockFileSystem.h"

#include <mutex>

namespace Common::FileSystem
{
    // Not exposed in pluginapi headers
    extern std::unique_ptr<IFileSystem>& fileSystemStaticPointer();
}

namespace{
    std::mutex preventTestsToUseEachOtherFileSystemMock; // NOLINT
    using IFileSystemPtr = std::unique_ptr<Common::FileSystem::IFileSystem>;

    //  We can't create our own instance, so we have to keep the original
    IFileSystemPtr GL_ORIGINAL_FILESYSTEM; // NOLINT


    void replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> pointerToReplace)
    {
        IFileSystemPtr& globalRef = Common::FileSystem::fileSystemStaticPointer();
        if (!GL_ORIGINAL_FILESYSTEM)
        {
            GL_ORIGINAL_FILESYSTEM = std::move(globalRef);
        }
        globalRef = std::move(pointerToReplace);
    }

    void restoreFileSystem()
    {
        IFileSystemPtr& globalRef = Common::FileSystem::fileSystemStaticPointer();
        assert(GL_ORIGINAL_FILESYSTEM);
        globalRef = std::move(GL_ORIGINAL_FILESYSTEM);
    }

}

namespace Tests
{
    ScopedReplaceFileSystem::ScopedReplaceFileSystem() : m_guard{preventTestsToUseEachOtherFileSystemMock}
    {
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