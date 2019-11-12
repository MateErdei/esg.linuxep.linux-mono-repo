/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Reactor/IReactor.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "ZmqCheckerMessageHandler.h"

#include <string>
#include <vector>

namespace zmqchecker
{
    class Server
            {
    public:
        Server(const std::string &socketAddress, bool captureSignals);

        void run();

        void join();

    private:
        std::string m_socketAddress;
        std::unique_ptr<Common::Reactor::IReactor> m_reactor;
        std::unique_ptr<ZmqCheckerMessageHandler> m_eventHandlerPtr;
        std::unique_ptr<Common::Reactor::IShutdownListener> m_shutdownListener;
        bool m_captureSignals;
        Common::ZMQWrapperApi::IContextSharedPtr m_ContextSharedPtr;
    };
}