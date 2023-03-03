// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "FilePermissionsReplaceAndRestore.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FileSystem;

class MockFilePermissions : public Common::FileSystem::IFilePermissions
{
public:
    MOCK_CONST_METHOD2(chmod, void(const Path& path, __mode_t mode));
    MOCK_CONST_METHOD3(chown, void(const Path& path, const std::string& user, const std::string& group));
    MOCK_CONST_METHOD3(chown, void(const Path& path, uid_t userId, gid_t groupId));
    MOCK_CONST_METHOD3(lchown, void(const Path& path, uid_t userId, gid_t groupId));
    MOCK_CONST_METHOD1(getGroupId, gid_t(const std::string& groupString));
    MOCK_CONST_METHOD1(getGroupName, std::string(const Path& filePath));
    MOCK_CONST_METHOD1(getUserIdOfDirEntry, uid_t (const Path& filePath));
    MOCK_CONST_METHOD1(getGroupIdOfDirEntry, gid_t (const Path& filePath));
    MOCK_CONST_METHOD1(getGroupName, std::string(const gid_t& groupId));
    MOCK_CONST_METHOD1(getUserId, uid_t(const std::string& userString));
    MOCK_CONST_METHOD1(getUserAndGroupId, std::pair<uid_t, gid_t>(const std::string& userString));
    MOCK_CONST_METHOD1(getUserName, std::string(const uid_t& userId));
    MOCK_CONST_METHOD1(getUserName, std::string(const Path& filePath));
    MOCK_CONST_METHOD1(getFilePermissions, mode_t(const Path& filePath));
    MOCK_CONST_METHOD0(getAllGroupNamesAndIds, std::map<std::string, gid_t>());
    MOCK_CONST_METHOD0(getAllUserNamesAndIds, std::map<std::string, uid_t>());
};

class IgnoreFilePermissions
{
public:
    IgnoreFilePermissions()
    {
        auto mockFilePermissions = new NiceMock<MockFilePermissions>();
        ON_CALL(*mockFilePermissions, chmod(_, _)).WillByDefault(Return());
        ON_CALL(*mockFilePermissions, chown(A<const Path&>(), A<const std::string&>(), A<const std::string&>())).WillByDefault(Return());

        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    }
    ~IgnoreFilePermissions()
    {
        Tests::restoreFilePermissions();
    }
};
