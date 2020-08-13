/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/mountinfo/ISystemPathsFactory.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class SystemPathsFactory : public ISystemPathsFactory
    {
    public:
        [[nodiscard]] mountinfo::ISystemPathsSharedPtr createSystemPaths() const override;
    };
}