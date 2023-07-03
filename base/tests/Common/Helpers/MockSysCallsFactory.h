// Copyright 2020-2023, Sophos Limited.  All rights reserved.

#include "Common/SystemCallWrapper/SystemCallWrapperFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::SystemCallWrapper;

class MockSystemCallWrapperFactory : public ISystemCallWrapperFactory
{
public:
    MOCK_METHOD(ISystemCallWrapperSharedPtr, createSystemCallWrapper, (), (override));
};
