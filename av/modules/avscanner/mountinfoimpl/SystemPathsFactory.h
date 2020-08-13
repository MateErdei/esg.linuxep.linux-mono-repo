/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/avscannerimpl/ISystemPathsFactory.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class SystemPathsFactory : public ISystemPathsFactory
    {
    public:
        [[nodiscard]] mountinfo::ISystemPathsSharedPtr createSystemPaths() const override;
    };
}