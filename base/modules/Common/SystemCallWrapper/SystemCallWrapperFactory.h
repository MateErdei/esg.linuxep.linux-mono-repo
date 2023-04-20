// Copyright 2020 Sophos Limited. All rights reserved.

#pragma once

#include "ISystemCallWrapperFactory.h"

#include <memory>

namespace Common::SystemCallWrapper
{
    class SystemCallWrapperFactory : public ISystemCallWrapperFactory
    {
    public:
        ISystemCallWrapperSharedPtr createSystemCallWrapper() override;
    };
}