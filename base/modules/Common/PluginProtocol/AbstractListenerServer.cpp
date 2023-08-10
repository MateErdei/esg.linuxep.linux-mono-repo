// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "AbstractListenerServer.h"

#include "Logger.h"

#include "Common/PluginApi/ApiException.h"
#include "Common/Reactor/IReactor.h"
namespace Common
{
    namespace PluginProtocol
    {
        AbstractListenerServer::AbstractListenerServer(
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
            ARMSHUTDOWNPOLICY armshutdownpolicy) :
            m_ireadWrite(std::move(ireadWrite))
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
                LOGDEBUG("Received request");
                DataMessage message = protocol.deserialize(data);
                DataMessage replyMessage = process(message);
                auto replyData = protocol.serialize(replyMessage);
                m_ireadWrite->write(replyData);
            }
            catch (const Common::PluginApi::ApiException& apiException)
            {
                LOGDEBUG("Failed, Send invalid as answer" << apiException.what());
                // Send a reply when de/serialisation fails to stop blocking on socket.
                m_ireadWrite->write(data_t{ "INVALID" });
                throw;
            }
        }

        void AbstractListenerServer::start() { m_reactor->start(); }

        void AbstractListenerServer::stop() { m_reactor->stop(); }

        void AbstractListenerServer::stopAndJoin()
        {
            m_reactor->stop();
            m_reactor->join();
        }

        void AbstractListenerServer::notifyShutdownRequested() { onShutdownRequested(); }
    } // namespace PluginProtocol
} // namespace Common