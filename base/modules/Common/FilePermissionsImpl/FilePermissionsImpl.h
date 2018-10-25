/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FilePermissions/IFilePermissions.h"

namespace Common
{
    namespace FilePermissions
    {
        class FilePermissionsImpl : public IFilePermissions
        {
        public:
            FilePermissionsImpl() = default;


            void sophosChmod(const Path& path, __mode_t mode) const override;

            void sophosChown(const Path& path, const std::string& user, const std::string& groupString) const override;

            struct group* sophosGetgrnam(const std::string& groupString) const override;

        };
        /** To be used in tests only */
        using IFilePermissionsPtr = std::unique_ptr<Common::FilePermissions::IFilePermissions>;
        void replaceFilePermissions(IFilePermissionsPtr);
    }
}