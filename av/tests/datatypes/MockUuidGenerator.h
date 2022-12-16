// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/IUuidGenerator.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockUuidGenerator : public datatypes::IUuidGenerator
    {
    public:
        MOCK_METHOD(std::string, generate, (), (const, override));
    };
} // namespace
