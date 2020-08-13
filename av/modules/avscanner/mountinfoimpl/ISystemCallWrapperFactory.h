/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapper.h"

#include <memory>

namespace avscanner::mountinfoimpl
{
    class ISystemCallWrapperFactory
    {
    public:
        virtual mountinfoimpl::ISystemCallWrapperSharedPtr createSystemCallWrapper() = 0;
    };

    using ISystemCallWrapperFactorySharedPtr = std::shared_ptr<ISystemCallWrapperFactory>;
}