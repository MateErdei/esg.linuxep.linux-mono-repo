/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/mountinfo/ISystemPaths.h"

namespace avscanner::avscannerimpl
{
    class ISystemPathsFactory
    {
    public:
        [[nodiscard]] virtual mountinfo::ISystemPathsSharedPtr createSystemPaths() const = 0;
    };

    using ISystemPathsFactorySharedPtr = std::shared_ptr<ISystemPathsFactory>;
}
