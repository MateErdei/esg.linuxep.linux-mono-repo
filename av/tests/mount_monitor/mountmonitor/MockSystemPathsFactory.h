//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "mount_monitor/mountinfo/ISystemPathsFactory.h"
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockSystemPathsFactory : public mount_monitor::mountinfo::ISystemPathsFactory
    {
    public:
        MOCK_METHOD(mount_monitor::mountinfo::ISystemPathsSharedPtr, createSystemPaths, (), (const));
    };
}