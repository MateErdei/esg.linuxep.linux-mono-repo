// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "FilePermissionsReplaceAndRestore.h"

#include "modules/Common/FileSystem/IFilePermissions.h"
#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FileSystem;

class MockFilePermissions : public Common::FileSystem::IFilePermissions
{
public:
    MOCK_METHOD(void, chmod, (const Path& path, __mode_t mode), (const, override));
    MOCK_METHOD(void, chown, (const Path& path, const std::string& user, const std::string& group), (const, override));
    MOCK_METHOD(void, chown, (const Path& path, uid_t userId, gid_t groupId), (const, override));
    MOCK_METHOD(void, lchown, (const Path& path, uid_t userId, gid_t groupId), (const, override));
    MOCK_METHOD(gid_t, getGroupId, (const std::string& groupString), (const, override));
    MOCK_METHOD(std::string, getGroupName, (const Path& filePath), (const, override));
    MOCK_METHOD(uid_t, getUserIdOfDirEntry, (const Path& filePath), (const, override));
    MOCK_METHOD(gid_t, getGroupIdOfDirEntry, (const Path& filePath), (const, override));
    MOCK_METHOD(std::string, getGroupName, (const gid_t& groupId), (const, override));
    MOCK_METHOD(uid_t, getUserId, (const std::string& userString), (const, override));
    MOCK_METHOD((std::pair<uid_t, gid_t>), getUserAndGroupId, (const std::string& userString), (const, override));
    MOCK_METHOD(std::string, getUserName, (const uid_t& userId), (const, override));
    MOCK_METHOD(std::string, getUserName, (const Path& filePath), (const, override));
    MOCK_METHOD(mode_t, getFilePermissions, (const Path& filePath), (const, override));
    MOCK_METHOD((std::map<std::string, gid_t>), getAllGroupNamesAndIds, (), (const, override));
    MOCK_METHOD((std::map<std::string, uid_t>), getAllUserNamesAndIds, (), (const, override));
    MOCK_METHOD(cap_t, getFileCapabilities, (const Path& filePath), (const, override));
    MOCK_METHOD(void, setFileCapabilities, (const Path& filePath, const cap_t& capabilities), (const, override));
};

class IgnoreFilePermissions
{
public:
    IgnoreFilePermissions()
    {
        auto mockFilePermissions = std::make_unique<NiceMock<MockFilePermissions>>();
        ON_CALL(*mockFilePermissions, chmod(_, _)).WillByDefault(Return());
        ON_CALL(*mockFilePermissions, chown(A<const Path&>(), A<const std::string&>(), A<const std::string&>())).WillByDefault(Return());
        Tests::replaceFilePermissions(std::move(mockFilePermissions));
    }
    ~IgnoreFilePermissions()
    {
        Tests::restoreFilePermissions();
    }
};
