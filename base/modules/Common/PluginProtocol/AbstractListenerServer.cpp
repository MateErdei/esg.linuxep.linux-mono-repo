/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractListenerServer.h"

#include "Common/ReactorImpl/ReactorImpl.h"

namespace Common
{
    namespace PluginProtocol
    {
        AbstractListenerServer::AbstractListenerServer(std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                                                       ARMSHUTDOWNPOLICY armshutdownpolicy)
        : m_ireadWrite(move(ireadWrite))
        {
            m_reactor = Common::Reactor::createReactor();
            m_reactor->addListener(m_ireadWrite.get(), this);
            if ( armshutdownpolicy == ARMSHUTDOWNPOLICY::HANDLESHUTDOWN)
            {
                m_reactor->armShutdownListener(this);
            }

        }

        void AbstractListenerServer::messageHandler( std::vector<std::string> data)
        {
            Protocol protocol;
            DataMessage message = protocol.deserialize(data);
            DataMessage replyMessage = process(message);
            auto replyData =  protocol.serialize(replyMessage);
            m_ireadWrite->write(replyData);
        }


        void AbstractListenerServer::start()
        {
            m_reactor->start();
        }

        void AbstractListenerServer::stop()
        {
            m_reactor->stop();
        }

        void AbstractListenerServer::notifyShutdownRequested()
        {
            onShutdownRequested();
        }
    }
}