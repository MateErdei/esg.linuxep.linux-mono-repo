//
// Created by pair on 26/06/18.
//

#include "FakeServer.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ReactorImpl/GenericShutdownListener.h"
#include "Common/ReactorImpl/ReactorImpl.h"
FakeServer::FakeServer(const std::string &socketAddress, bool captureSignals)
    : m_socketAddress(socketAddress),
      m_reactor(std::unique_ptr<Common::Reactor::IReactor>(new Common::Reactor::ReactorImpl())),
      m_captureSignals(captureSignals)
{
}

void FakeServer::run(Common::ZeroMQWrapper::IContext& iContext)
{
    auto replier = iContext.getReplier();
    replier->listen(m_socketAddress);
    Common::ZeroMQWrapper::IReadable * readable = replier.get();
    m_testListener = std::unique_ptr<TestListener>(new TestListener(std::move(replier)));

    m_reactor->addListener(readable, m_testListener.get());

    if( m_captureSignals)
    {
        m_shutdownListener = std::unique_ptr<Common::Reactor::IShutdownListener>( new Common::Reactor::GenericShutdownListener([](){}));
        m_reactor->armShutdownListener(m_shutdownListener.get());
    }

    m_reactor->start();
}

void FakeServer::join()
{
    m_reactor->join();
}
