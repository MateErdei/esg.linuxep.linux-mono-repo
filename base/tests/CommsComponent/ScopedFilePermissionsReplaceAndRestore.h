/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystemImpl/FilePermissionsImpl.h"
#include <mutex>

namespace Tests
{
    void replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>);
    void restoreFilePermissions();

    class NullFilePermission : public Common::FileSystem::FilePermissionsImpl
    {
    public:
        void chown(const Path&, const std::string&, const std::string&) const override {}
        void chmod(const Path&, __mode_t) const override {}
    };

    struct ScopedReplaceFilePermissions{
        enum UseNullFilePermission{YES}; 
        std::lock_guard<std::mutex> m_guard; 
        ScopedReplaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>); 
        ScopedReplaceFilePermissions(); 
        ScopedReplaceFilePermissions(UseNullFilePermission); 
        void replace(std::unique_ptr<Common::FileSystem::IFilePermissions>); 
        ~ScopedReplaceFilePermissions(); 
    };

} // namespace Tests
