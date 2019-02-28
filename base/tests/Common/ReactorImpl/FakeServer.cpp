/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServer.h"

#include "Common/ReactorImpl/GenericShutdownListener.h"
#include "Common/ReactorImpl/ReactorImpl.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
FakeServer::FakeServer(const std::string& socketAddress, bool captureSignals) :
    m_socketAddress(socketAddress),
    m_reactor(Common::Reactor::createReactor()),
    m_captureSignals(captureSignals)
{
}

void FakeServer::run(Common::ZMQWrapperApi::IContext& iContext)
{
    auto replier = iContext.getReplier();
    replier->listen(m_socketAddress);
    Common::ZeroMQWrapper::IReadable* readable = replier.get();
    m_testListener = std::unique_ptr<TestListener>(new TestListener(std::move(replier)));

    m_reactor->addListener(readable, m_testListener.get());

    if (m_captureSignals)
    {
        m_shutdownListener = std::unique_ptr<Common::Reactor::IShutdownListener>(
            new Common::ReactorImpl::GenericShutdownListener([]() {}));
        m_reactor->armShutdownListener(m_shutdownListener.get());
    }

    m_reactor->start();
}

void FakeServer::join()
{
    m_reactor->join();
}
