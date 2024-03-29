// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SocketImpl.h"

#include "Common/ZeroMQWrapper/ISocketPublisher.h"

namespace Common::ZeroMQWrapperImpl
{
    class SocketPublisherImpl : public SocketImpl, public virtual Common::ZeroMQWrapper::ISocketPublisher
    {
    public:
        explicit SocketPublisherImpl(ContextHolderSharedPtr context);

        void write(const std::vector<std::string>& data) override;
    };
} // namespace Common::ZeroMQWrapperImpl
