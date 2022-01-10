/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapperFactory.h"

#include <memory>

namespace datatypes
{
    class SystemCallWrapperFactory : public ISystemCallWrapperFactory
    {
    public:
        ISystemCallWrapperSharedPtr createSystemCallWrapper() override;
    };
}