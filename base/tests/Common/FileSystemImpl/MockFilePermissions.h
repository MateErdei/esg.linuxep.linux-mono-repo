/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include "Common/FileSystem/IFilePermissions.h"
#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FileSystem;

class MockFilePermissions: public Common::FileSystem::IFilePermissions
{
public:
    MOCK_CONST_METHOD2(chmod, void(const Path& path, __mode_t mode));
    MOCK_CONST_METHOD3(chown, void(const Path& path, const std::string& user, const std::string& group));
    MOCK_CONST_METHOD1(getGroupId, int (const std::string& groupString));
};
