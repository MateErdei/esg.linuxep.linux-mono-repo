/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include "Common/FilePermissions/IFilePermissions.h"
#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FilePermissions;

class MockFilePermissions: public Common::FilePermissions::IFilePermissions
{
public:
    MOCK_CONST_METHOD2(sophosChmod, void(const Path& path, __mode_t mode));
    MOCK_CONST_METHOD3(sophosChown, void(const Path& path, const std::string& user, const std::string& group));
    MOCK_CONST_METHOD1(sophosGetgrnam, struct group* (const std::string& groupString));
};
