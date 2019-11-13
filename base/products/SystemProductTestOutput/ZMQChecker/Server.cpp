/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Server.h"

#include "Common/ReactorImpl/GenericShutdownListener.h"
#include "Common/ReactorImpl/ReactorImpl.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"

#include <iostream>

namespace zmqchecker
{
    Server::Server(const std::string &socketAddress, bool captureSignals, bool ignoreRequests) :
            m_socketAddress(socketAddress),
            m_reactor(Common::Reactor::createReactor()),
            m_captureSignals(captureSignals),
            m_ContextSharedPtr(Common::ZMQWrapperApi::createContext()),
            m_ignoreRequests(ignoreRequests)
    {
        std::cout << "server created at: " << socketAddress << std::endl;
    }

    void Server::run()
    {
        std::cout << "getting replier" << std::endl;
        auto replier = m_ContextSharedPtr->getReplier();

        replier->listen(m_socketAddress);
        Common::ZeroMQWrapper::IReadable *readable = replier.get();
        m_eventHandlerPtr = std::make_unique<ZmqCheckerMessageHandler>(std::move(replier), m_ignoreRequests);
        std::cout << "listening at socket address" << std::endl;
        m_reactor->addListener(readable, m_eventHandlerPtr.get());

        if (m_captureSignals) {
            m_shutdownListener = std::unique_ptr<Common::Reactor::IShutdownListener>(
                    new Common::ReactorImpl::GenericShutdownListener([]() {}));
            m_reactor->armShutdownListener(m_shutdownListener.get());
        }
        std::cout << "starting server" << std::endl;
        m_reactor->start();
    }

    void Server::join() {
        m_reactor->join();
    }
}