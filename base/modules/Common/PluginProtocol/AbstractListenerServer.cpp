/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractListenerServer.h"

#include "Common/ReactorImpl/ReactorImpl.h"
#include "Common/PluginApi/ApiException.h"


namespace Common
{
    namespace PluginProtocol
    {
        AbstractListenerServer::AbstractListenerServer(
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
            ARMSHUTDOWNPOLICY armshutdownpolicy) :
            m_ireadWrite(move(ireadWrite))
        {
            m_reactor = Common::Reactor::createReactor();
            m_reactor->addListener(m_ireadWrite.get(), this);
            if (armshutdownpolicy == ARMSHUTDOWNPOLICY::HANDLESHUTDOWN)
            {
                m_reactor->armShutdownListener(this);
            }
        }

        void AbstractListenerServer::messageHandler(std::vector<std::string> data)
        {
            Protocol protocol;
            try
            {
                DataMessage message = protocol.deserialize(data);
                DataMessage replyMessage = process(message);
                auto replyData = protocol.serialize(replyMessage);
                m_ireadWrite->write(replyData);
            }
            catch (const Common::PluginApi::ApiException & apiException)
            {
                //Send a reply when de/serialisation fails to stop blocking on socket.
                m_ireadWrite->write(data_t{"INVALID"});
                throw;
            }
        }

        void AbstractListenerServer::start() { m_reactor->start(); }

        void AbstractListenerServer::stop() { m_reactor->stop(); }

        void AbstractListenerServer::notifyShutdownRequested() { onShutdownRequested(); }
    } // namespace PluginProtocol
} // namespace Common