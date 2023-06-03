// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ZeroMQWrapper/IPollerFactory.h"

#include <gmock/gmock.h>

class MockPollerFactory : public Common::ZeroMQWrapper::IPollerFactory
{
public:
    MOCK_METHOD(Common::ZeroMQWrapper::IPollerPtr, create, (), (override, const));
};