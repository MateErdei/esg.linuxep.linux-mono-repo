/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <cassert>
#include "Common/PluginApi/ApiException.h"
#include "PluginServerCallbackHandler.h"
#include "PluginServerCallback.h"


namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        PluginServerCallbackHandler::PluginServerCallbackHandler(
                std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                std::shared_ptr<PluginCommunication::IPluginServerCallback> serverCallback) :
                AbstractListenerServer(std::move(ireadWrite), ARMSHUTDOWNPOLICY::DONOTARM),
                m_messageBuilder("NotUsed", ProtocolSerializerFactory::ProtocolVersion),
                m_serverCallback(std::move(serverCallback))
        {
        }

        DataMessage PluginServerCallbackHandler::process(const DataMessage& request) const
        {
            try
            {
                std::string policyXml;
                switch (request.Command)
                {
                    case Commands::PLUGIN_SEND_EVENT:
                        m_serverCallback->receivedSendEvent(request.ApplicationId,
                                                            m_messageBuilder.requestExtractEvent(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Commands::PLUGIN_SEND_STATUS:
                        m_serverCallback->receivedChangeStatus(request.ApplicationId,
                                                               m_messageBuilder.requestExtractStatus(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Commands::PLUGIN_QUERY_CURRENT_POLICY:
                        policyXml = m_serverCallback->receivedGetPolicy(request.ApplicationId);
                        return m_messageBuilder.replyCurrentPolicy(request, policyXml);
                    case Commands::PLUGIN_SEND_REGISTER:
                        m_serverCallback->receivedRegisterWithManagementAgent(request.PluginName);
                        return m_messageBuilder.replyAckMessage(request);
                    default:
                        return m_messageBuilder.replySetErrorIfEmpty(request, "Request not supported");
                }
            }
            catch (Common::PluginApi::ApiException& ex)
            {
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }
            catch (std::exception& ex)
            {
                return m_messageBuilder.replySetErrorIfEmpty(request, ex.what());
            }
        }

        void PluginServerCallbackHandler::onShutdownRequested()
        {
            /**not used **/
        }

        void PluginServerCallbackHandler::setStatusReceiver(
                std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
        {
            auto test = dynamic_cast<PluginServerCallback*>(m_serverCallback.get());
            assert(test != nullptr);
            test->setStatusReceiver(statusReceiver);
        }
    }
}
