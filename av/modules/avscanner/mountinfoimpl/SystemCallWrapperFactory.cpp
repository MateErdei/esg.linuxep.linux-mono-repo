/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCallWrapperFactory.h"

#include "SystemCallWrapper.h"

using namespace avscanner::avscannerimpl;

std::shared_ptr<ISystemCallWrapper> SystemCallWrapperFactory::createSystemCallWrapper()
{
    return std::make_shared<SystemCallWrapper>();
}