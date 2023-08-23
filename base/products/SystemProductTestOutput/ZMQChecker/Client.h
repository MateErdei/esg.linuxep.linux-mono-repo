// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/IReadable.h"

#include <string>
#include <vector>

namespace zmqchecker
{
    class Client
    {
    public:
        Client(const std::string& address, int timeout);
        ~Client();
        Common::ZeroMQWrapper::IReadable::data_t requestReply(
            const Common::ZeroMQWrapper::IReadable::data_t& request,
            bool ignoreReply = false);

    private:
        std::unique_ptr<Common::ZeroMQWrapper::ISocketRequester> m_socketRequester;
        Common::ZMQWrapperApi::IContextSharedPtr m_iContextSharedPtr;
    };
} // namespace zmqchecker