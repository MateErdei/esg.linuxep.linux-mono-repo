/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapper.h"
#include "SusiWrapperFactory.h"

std::shared_ptr<ISusiWrapper> SusiWrapperFactory::createSusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig)
{
    return std::make_shared<SusiWrapper>(runtimeConfig, scannerConfig);
}