/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemPathsFactory.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class SystemPathsFactory : public ISystemPathsFactory
    {
    public:
        std::shared_ptr<ISystemPaths> createSystemPaths();
    };
}