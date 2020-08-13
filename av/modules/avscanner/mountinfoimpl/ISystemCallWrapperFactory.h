/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapper.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class ISystemCallWrapperFactory
    {
    public:
        virtual mountinfoimpl::ISystemCallWrapperSharedPtr createSystemCallWrapper() = 0;
    };
}