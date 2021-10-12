/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFilePermissions.h"

#include <mutex>

namespace Tests
{
    void replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>);
    void restoreFilePermissions();
    struct ScopedReplaceFilePermissions{
        std::lock_guard<std::mutex> m_guard;
        ScopedReplaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>);
        ScopedReplaceFilePermissions();
        void replace(std::unique_ptr<Common::FileSystem::IFilePermissions>);
        ~ScopedReplaceFilePermissions();
    };
} // namespace Tests
