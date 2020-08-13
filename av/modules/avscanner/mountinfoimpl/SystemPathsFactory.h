/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/mountinfo/ISystemPathsFactory.h"

#include <memory>

namespace avscanner::mountinfoimpl
{
    class SystemPathsFactory : public avscanner::mountinfo::ISystemPathsFactory
    {
    public:
        [[nodiscard]] mountinfo::ISystemPathsSharedPtr createSystemPaths() const override;
    };
}