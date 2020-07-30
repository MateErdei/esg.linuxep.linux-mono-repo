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
        virtual std::shared_ptr<ISystemCallWrapper> createSystemCallWrapper() = 0;
    };
}