/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemCallWrapperFactory.h"

#include "SystemCallWrapper.h"

using namespace avscanner::mountinfoimpl;

ISystemCallWrapperSharedPtr SystemCallWrapperFactory::createSystemCallWrapper()
{
    return std::make_shared<SystemCallWrapper>();
}