// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    class MockSusiScannerFactory : public threat_scanner::IThreatScannerFactory
    {
        public:
            MockSusiScannerFactory()
            {
                ON_CALL(*this, update).WillByDefault(Return(true));
                ON_CALL(*this, reload).WillByDefault(Return(true));
                ON_CALL(*this, susiIsInitialized).WillByDefault(Return(true));
            };

            MOCK_METHOD(threat_scanner::IThreatScannerPtr, createScanner, (bool scanArchives, bool scanimages), (override));
            MOCK_METHOD(bool, update, (), (override));
            MOCK_METHOD(bool, reload, (), (override));
            MOCK_METHOD(void, shutdown, (), (override));
            MOCK_METHOD(bool, susiIsInitialized, (), (override));
            MOCK_METHOD(bool, updateSusiConfig, ());
    };

}