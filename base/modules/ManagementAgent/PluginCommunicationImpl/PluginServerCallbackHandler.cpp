/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginServerCallbackHandler.h"

#include "PluginServerCallback.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/NoPolicyAvailableException.h>

#include <cassert>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        PluginServerCallbackHandler::PluginServerCallbackHandler(
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
            std::shared_ptr<PluginCommunication::IPluginServerCallback> serverCallback) :
            AbstractListenerServer(std::move(ireadWrite), ARMSHUTDOWNPOLICY::DONOTARM),
            m_messageBuilder("NotUsed"),
            m_serverCallback(std::move(serverCallback))
        {
        }

        DataMessage PluginServerCallbackHandler::process(const DataMessage& request) const
        {
            try
            {
                std::string policyXml;
                switch (request.m_command)
                {
                    case Commands::PLUGIN_SEND_EVENT:
                        m_serverCallback->receivedSendEvent(
                            request.m_applicationId, m_messageBuilder.requestExtractEvent(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Commands::PLUGIN_SEND_STATUS:
                        m_serverCallback->receivedChangeStatus(
                            request.m_applicationId, m_messageBuilder.requestExtractStatus(request));
                        return m_messageBuilder.replyAckMessage(request);
                    case Commands::PLUGIN_QUERY_CURRENT_POLICY:
                    {
                        if (m_serverCallback->receivedGetPolicyRequest(request.m_applicationId))
                        {
                            return m_messageBuilder.replyAckMessage(request);
                        }
                        else
                        {
                            return m_messageBuilder.replySetErrorIfEmpty(
                                request, Common::PluginApi::NoPolicyAvailableException::NoPolicyAvailable);
                        }
                    }
                    case Commands::PLUGIN_SEND_REGISTER:
                        m_serverCallback->receivedRegisterWithManagementAgent(request.m_pluginName);
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
        { /**not used **/
        }

        void PluginServerCallbackHandler::setStatusReceiver(
            std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
        {
            auto serverCallbackAsPluginServerCallback = dynamic_cast<PluginServerCallback*>(m_serverCallback.get());
            assert(serverCallbackAsPluginServerCallback != nullptr);

            if (serverCallbackAsPluginServerCallback != nullptr) // for non-debug builds, don't crash
            {
                serverCallbackAsPluginServerCallback->setStatusReceiver(statusReceiver);
            }
        }

        void PluginServerCallbackHandler::setEventReceiver(
            std::shared_ptr<PluginCommunication::IEventReceiver>& receiver)
        {
            auto serverCallbackAsPluginServerCallback = dynamic_cast<PluginServerCallback*>(m_serverCallback.get());
            assert(serverCallbackAsPluginServerCallback != nullptr);

            if (serverCallbackAsPluginServerCallback != nullptr) // for non-debug builds, don't crash
            {
                serverCallbackAsPluginServerCallback->setEventReceiver(receiver);
            }
        }

        void PluginServerCallbackHandler::setPolicyReceiver(
            std::shared_ptr<PluginCommunication::IPolicyReceiver>& policyReceiver)
        {
            auto serverCallbackAsPluginServerCallback = dynamic_cast<PluginServerCallback*>(m_serverCallback.get());
            assert(serverCallbackAsPluginServerCallback != nullptr);

            if (serverCallbackAsPluginServerCallback != nullptr) // for non-debug builds, don't crash
            {
                serverCallbackAsPluginServerCallback->setPolicyReceiver(policyReceiver);
            }
        }
    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
