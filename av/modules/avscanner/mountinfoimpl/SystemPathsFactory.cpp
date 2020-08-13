/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemPathsFactory.h"

#include "avscanner/mountinfoimpl/SystemPaths.h"

using namespace avscanner::mountinfoimpl;

avscanner::mountinfo::ISystemPathsSharedPtr SystemPathsFactory::createSystemPaths() const
{
    return std::make_shared<SystemPaths>();
}
