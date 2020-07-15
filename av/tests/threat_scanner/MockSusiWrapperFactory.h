/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/ISusiWrapperFactory.h"

class MockSusiWrapperFactory : public ISusiWrapperFactory
{
public:
    MOCK_METHOD1(createSusiWrapper, std::shared_ptr<ISusiWrapper>(const std::string& /*scannerConfig*/));
};