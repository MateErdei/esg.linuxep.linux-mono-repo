/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemPaths.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class ISystemPathsFactory
    {
    public:
        virtual std::shared_ptr<ISystemPaths> createSystemPaths() = 0;
    };
}