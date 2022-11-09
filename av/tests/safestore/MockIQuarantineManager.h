// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include "safestore/QuarantineManager/IQuarantineManager.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace safestore::QuarantineManager;

class MockIQuarantineManager : public IQuarantineManager
{
public:
    MOCK_METHOD0(initialise, void());
    MOCK_METHOD0(getState, safestore::QuarantineManager::QuarantineManagerState());
    MOCK_METHOD1(setState, void(const safestore::QuarantineManager::QuarantineManagerState&));
    MOCK_METHOD0(deleteDatabase, bool());
    MOCK_METHOD5(
        quarantineFile,
        common::CentralEnums::QuarantineResult(
            const std::string&,
            const std::string&,
            const std::string&,
            const std::string&,
            datatypes::AutoFd));
    MOCK_METHOD0(rescanDatabase, void());
};
