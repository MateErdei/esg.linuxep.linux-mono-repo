/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemPathsFactory.h"

#include "SystemPaths.h"

using namespace mount_monitor::mountinfoimpl;

mount_monitor::mountinfo::ISystemPathsSharedPtr SystemPathsFactory::createSystemPaths() const
{
    return std::make_shared<SystemPaths>();
}
