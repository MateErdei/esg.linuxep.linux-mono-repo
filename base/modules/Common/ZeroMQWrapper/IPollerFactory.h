// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IPoller.h"

namespace Common::ZeroMQWrapper
{
    class IPollerFactory
    {
    public:
        virtual ~IPollerFactory() = default;

        virtual IPollerPtr create() const = 0;
    };

    IPollerFactory& pollerFactory();
    void replacePollerFactory(std::unique_ptr<IPollerFactory> pollerFactory);
    void restorePollerFactory();
} // namespace Common::ZeroMQWrapper
