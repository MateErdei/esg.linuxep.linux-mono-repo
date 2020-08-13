/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCallWrapperFactory.h"

#include "SystemCallWrapper.h"

using namespace avscanner::mountinfoimpl;
using namespace avscanner::avscannerimpl;

ISystemCallWrapperSharedPtr SystemCallWrapperFactory::createSystemCallWrapper()
{
    return std::make_shared<SystemCallWrapper>();
}