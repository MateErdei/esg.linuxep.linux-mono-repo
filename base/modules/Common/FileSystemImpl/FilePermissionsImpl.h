/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFilePermissions.h"

namespace Common
{
    namespace FileSystem
    {
        class FilePermissionsImpl : public IFilePermissions
        {
        public:
            FilePermissionsImpl() = default;


            void chmod(const Path& path, __mode_t mode) const override;

            void chown(const Path& path, const std::string& user, const std::string& groupString) const override;

            int getgrnam(const std::string& groupString) const override;

        };
        /** To be used in tests only */
        using IFilePermissionsPtr = std::unique_ptr<Common::FileSystem::IFilePermissions>;
        void replaceFilePermissions(IFilePermissionsPtr);
    }
}