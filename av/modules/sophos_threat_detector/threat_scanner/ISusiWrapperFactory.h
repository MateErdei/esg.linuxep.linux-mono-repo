/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapper.h"

#include <memory>

class ISusiWrapperFactory
{
public:
    virtual std::shared_ptr<ISusiWrapper> createSusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig) = 0;
};
