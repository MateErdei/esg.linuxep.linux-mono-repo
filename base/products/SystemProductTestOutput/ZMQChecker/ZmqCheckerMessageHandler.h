/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Reactor/ICallbackListener.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include <functional>

namespace zmqchecker
{
    class ZmqCheckerMessageHandler : public Common::Reactor::ICallbackListener
    {
    public:
        explicit ZmqCheckerMessageHandler(
                std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> socketReplier, bool ignoreRequests=false);

        void messageHandler(Common::ZeroMQWrapper::IReadable::data_t) override;

    private:
        std::unique_ptr<Common::ZeroMQWrapper::ISocketReplier> m_socketReplier;
        bool m_ignoreRequests;
    };
}