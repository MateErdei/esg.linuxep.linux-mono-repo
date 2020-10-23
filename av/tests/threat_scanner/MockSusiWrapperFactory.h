/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/ISusiWrapperFactory.h"

#include <gmock/gmock.h>

class MockSusiWrapperFactory : public threat_scanner::ISusiWrapperFactory
{
public:
    MOCK_METHOD1(createSusiWrapper, threat_scanner::ISusiWrapperSharedPtr(const std::string& /*scannerConfig*/));

    MOCK_METHOD0(update, bool());
};
