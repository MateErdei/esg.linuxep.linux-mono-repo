// Copyright 2020 Sophos Limited. All rights reserved.

#pragma once

#include "ISystemCallWrapper.h"

#include <memory>

namespace Common::SystemCallWrapper
{
    class ISystemCallWrapperFactory
    {
    public:
        virtual ~ISystemCallWrapperFactory() = default;
        virtual ISystemCallWrapperSharedPtr createSystemCallWrapper() = 0;
    };

    using ISystemCallWrapperFactorySharedPtr = std::shared_ptr<ISystemCallWrapperFactory>;
} // namespace Common::SystemCallWrapper