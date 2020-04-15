/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapperFactory.h"

class SusiWrapperFactory : public ISusiWrapperFactory
{
public:
    std::shared_ptr<ISusiWrapper> createSusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig) override;
};
