// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SocketImpl.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"

namespace Common::ZeroMQWrapperImpl
{
    class SocketRequesterImpl : public SocketImpl, public virtual Common::ZeroMQWrapper::ISocketRequester
    {
    public:
        explicit SocketRequesterImpl(ContextHolderSharedPtr context);

        std::vector<std::string> read() override;

        void write(const std::vector<std::string>& data) override;
    };
} // namespace Common::ZeroMQWrapperImpl
