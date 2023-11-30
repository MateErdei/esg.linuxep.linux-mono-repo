// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IPidLockFileUtils.h"

#include <gmock/gmock.h>

class MockLockFileHolder : public Common::FileSystem::ILockFileHolder
{
public:
    MOCK_METHOD(std::string, filePath, (), (const, override));
};
