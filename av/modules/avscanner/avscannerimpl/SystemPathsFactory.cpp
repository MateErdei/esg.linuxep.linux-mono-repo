/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemPathsFactory.h"

#include "SystemPaths.h"

using namespace avscanner::avscannerimpl;

std::shared_ptr<ISystemPaths> SystemPathsFactory::createSystemPaths()
{
    return std::make_shared<SystemPaths>();
}