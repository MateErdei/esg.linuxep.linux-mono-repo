/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapperFactory.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class SystemCallWrapperFactory : public mountinfoimpl::ISystemCallWrapperFactory
    {
    public:
        mountinfoimpl::ISystemCallWrapperSharedPtr createSystemCallWrapper() override;
    };
}