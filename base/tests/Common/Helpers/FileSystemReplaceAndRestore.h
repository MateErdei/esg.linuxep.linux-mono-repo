/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFileSystem.h"
#include <mutex>
namespace Tests
{
    void replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>);
    void restoreFileSystem();
    struct ScopedReplaceFileSystem{
        std::lock_guard<std::mutex> m_guard; 
        ScopedReplaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>); 
        ScopedReplaceFileSystem(); 
        void replace(std::unique_ptr<Common::FileSystem::IFileSystem>); 
        ~ScopedReplaceFileSystem(); 
    };
} // namespace Tests
