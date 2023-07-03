// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/SystemCallWrapper/ISystemCallWrapper.h"
#include <sys/capability.h>

namespace Common::FileSystem
{
    class FilePermissionsImpl : public IFilePermissions
    {
    public:
        explicit FilePermissionsImpl(Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCallWrapper = nullptr);

        void chmod(const Path& path, __mode_t mode) const override;

        void chown(const Path& path, const std::string& user, const std::string& groupString) const override;

        void chown(const Path& path, uid_t userId, gid_t groupId) const override;

        void lchown(const Path& path, uid_t userId, gid_t groupId) const override;

        gid_t getGroupId(const std::string& groupString) const override;

        std::string getGroupName(const gid_t& groupId) const override;

        std::string getGroupName(const Path& filePath) const override;

        uid_t getUserId(const std::string& userString) const override;

        std::pair<uid_t, gid_t> getUserAndGroupId(const std::string& userString) const override;

        std::string getUserName(const uid_t& userId) const override;

        std::string getUserName(const Path& filePath) const override;

        mode_t getFilePermissions(const Path& filePath) const override;

        std::map<std::string, gid_t> getAllGroupNamesAndIds() const override;

        uid_t getUserIdOfDirEntry(const Path& path) const override;

        gid_t getGroupIdOfDirEntry(const Path& path) const override;

        std::map<std::string, uid_t> getAllUserNamesAndIds() const override;

        cap_t getFileCapabilities(const Path& path) const override;

        void setFileCapabilities(const Path& path, const cap_t& capabilities) const override;

    private:
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_sysCallWrapper = nullptr;
    };
    std::unique_ptr<IFilePermissions>& filePermissionsStaticPointer();
} // namespace Common::FileSystem
