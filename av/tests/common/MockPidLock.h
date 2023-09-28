// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/PidLockFile.h"
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockPidLock : public common::IPidLockFile
    {
    public:
        MockPidLock()
        {

        }
    };
}