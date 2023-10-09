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

            gid_t getGroupId(const std::string& groupString) const override;

            std::string getGroupName(const gid_t& groupId) const override;

            std::string getGroupName(const Path& filePath) const override;

            uid_t getUserId(const std::string& userString) const override;

            std::pair<uid_t, gid_t> getUserAndGroupId(const std::string& userString) const override;


            std::string getUserName(const uid_t& userId) const override;

            std::string getUserName(const Path& filePath) const override;

            mode_t getFilePermissions(const Path& filePath) const override;
        };

        std::unique_ptr<IFilePermissions>& filePermissionsStaticPointer();
    } // namespace FileSystem
} // namespace Common
