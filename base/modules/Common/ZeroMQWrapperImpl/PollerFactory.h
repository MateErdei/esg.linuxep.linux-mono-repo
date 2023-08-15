// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ZeroMQWrapper/IPollerFactory.h"

namespace Common::ZeroMQWrapperImpl
{
    class PollerFactory : public Common::ZeroMQWrapper::IPollerFactory
    {
    public:
        [[nodiscard]] Common::ZeroMQWrapper::IPollerPtr create() const override;
    };
} // namespace Common::ZeroMQWrapperImpl
