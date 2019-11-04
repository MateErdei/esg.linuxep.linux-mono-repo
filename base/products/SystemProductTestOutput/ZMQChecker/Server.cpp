/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Server.h"

#include "Common/ReactorImpl/GenericShutdownListener.h"
#include "Common/ReactorImpl/ReactorImpl.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"


Server::Server(const std::string& socketAddress, bool captureSignals) :
        m_socketAddress(socketAddress),
        m_reactor(Common::Reactor::createReactor()),
        m_captureSignals(captureSignals),
        m_ContextSharedPtr(Common::ZMQWrapperApi::createContext())
{
}

void Server::run()
{

    auto replier = m_ContextSharedPtr->getReplier();
    replier->listen(m_socketAddress);
    Common::ZeroMQWrapper::IReadable* readable = replier.get();
    m_testListener = std::unique_ptr<ZmqCheckerMessageHandler>(new ZmqCheckerMessageHandler(std::move(replier), std::bind(&Common::ReactorImpl::ReactorImpl::stop, (dynamic_cast<Common::ReactorImpl::ReactorImpl*>(m_reactor.get())))));

    m_reactor->addListener(readable, m_testListener.get());

    if (m_captureSignals)
    {
        m_shutdownListener = std::unique_ptr<Common::Reactor::IShutdownListener>(
                new Common::ReactorImpl::GenericShutdownListener([]() {}));
        m_reactor->armShutdownListener(m_shutdownListener.get());
    }

    m_reactor->start();
}

void Server::join()
{
    m_reactor->join();
}
