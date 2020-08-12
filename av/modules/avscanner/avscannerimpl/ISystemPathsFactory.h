/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemPaths.h"

namespace avscanner::avscannerimpl
{
    class ISystemPathsFactory
    {
    public:
        [[nodiscard]] virtual ISystemPathsSharedPtr createSystemPaths() const = 0;
    };

    using ISystemPathsFactorySharedPtr = std::shared_ptr<ISystemPathsFactory>;
}
