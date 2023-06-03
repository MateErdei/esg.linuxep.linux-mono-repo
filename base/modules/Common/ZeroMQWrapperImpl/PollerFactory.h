// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "../ZeroMQWrapper/IPollerFactory.h"

namespace Common::ZeroMQWrapperImpl
{
    class PollerFactory : public Common::ZeroMQWrapper::IPollerFactory
    {
    public:
        Common::ZeroMQWrapper::IPollerPtr create() const override;
    };
} // namespace Common::ZeroMQWrapperImpl
