/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include <Common/FileSystem/IFilePermissions.h>

#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FileSystem;

class MockFilePermissions: public Common::FileSystem::IFilePermissions
{
public:
    MOCK_CONST_METHOD2(chmod, void(const Path& path, __mode_t mode));
    MOCK_CONST_METHOD3(chown, void(const Path& path, const std::string& user, const std::string& group));
    MOCK_CONST_METHOD1(getGroupId, gid_t (const std::string& groupString));
    MOCK_CONST_METHOD1(getGroupName, std::string (const gid_t& groupId));
    MOCK_CONST_METHOD1(getUserId, uid_t (const std::string& userString));
    MOCK_CONST_METHOD1(getUserName, std::string (const uid_t& userId));
};
