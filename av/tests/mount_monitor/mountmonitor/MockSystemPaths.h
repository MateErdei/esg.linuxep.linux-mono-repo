//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "mount_monitor/mountinfo/ISystemPaths.h"
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockSystemPaths : public mount_monitor::mountinfo::ISystemPaths
    {
    public:
        MOCK_METHOD(std::string, mountInfoFilePath, (), (const));
        MOCK_METHOD(std::string, cmdlineInfoFilePath, (), (const));
        MOCK_METHOD(std::string, findfsCmdPath, (), (const));
        MOCK_METHOD(std::string, mountCmdPath, (), (const));
    };
}