// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "ISettablePluginServerCallback.h"

#include <Common/PluginProtocol/AbstractListenerServer.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/ZeroMQWrapper/IReadWrite.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <ManagementAgent/HealthStatusImpl/HealthStatus.h>
#include <ManagementAgent/PluginCommunication/IPluginServerCallback.h>
#include <ManagementAgent/PluginCommunication/IPolicyReceiver.h>

using namespace Common::PluginProtocol;

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallbackHandler : public AbstractListenerServer
        {
        public:
            using ServerCallback_t = ManagementAgent::PluginCommunicationImpl::ISettablePluginServerCallback;
            using ServerCallbackPtr = std::shared_ptr<ServerCallback_t>;

            PluginServerCallbackHandler(
                std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                ServerCallbackPtr serverCallback,
                std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj);

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver);
            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver);
            void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver);
            void setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver);

        private:
            [[nodiscard]] DataMessage process(const DataMessage& request) const override;

            void onShutdownRequested() override;

            MessageBuilder m_messageBuilder;
            ServerCallbackPtr m_serverCallback;
            std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> m_healthStatusSharedObj;
        };

    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
