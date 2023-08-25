// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "tests/common/MockScanner.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    class MockSusiScannerFactory : public MockScannerFactory
    {
        public:
            MockSusiScannerFactory()
            {
                ON_CALL(*this, update).WillByDefault(Return(true));
                ON_CALL(*this, reload).WillByDefault(Return(true));
                ON_CALL(*this, susiIsInitialized).WillByDefault(Return(false));
            };
    };

}