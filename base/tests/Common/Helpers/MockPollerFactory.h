// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/ZeroMQWrapper/IPollerFactory.h"

#include <gmock/gmock.h>

class MockPollerFactory : public Common::ZeroMQWrapper::IPollerFactory
{
public:
    MOCK_METHOD(Common::ZeroMQWrapper::IPollerPtr, create, (), (override, const));
};