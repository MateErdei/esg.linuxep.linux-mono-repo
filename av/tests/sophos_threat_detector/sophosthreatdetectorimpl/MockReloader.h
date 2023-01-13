// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include "common/signals/IReloadable.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockReloader : public common::signals::IReloadable
    {
        public:
            MockReloader() {}

            MOCK_METHOD(void, update, (), (override));
            MOCK_METHOD(void, reload, (), (override));
            MOCK_METHOD(bool, updateSusiConfig, (), (override));
    };
}


