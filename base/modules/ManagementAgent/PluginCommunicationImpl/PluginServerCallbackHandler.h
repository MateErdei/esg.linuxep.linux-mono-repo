/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IPluginServerCallback.h"

#include <Common/PluginProtocol/AbstractListenerServer.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/ZeroMQWrapper/IReadWrite.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <ManagementAgent/PluginCommunication/IPolicyReceiver.h>

using namespace Common::PluginProtocol;

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallbackHandler : public AbstractListenerServer
        {
        public:
            PluginServerCallbackHandler(
                std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                std::shared_ptr<PluginCommunication::IPluginServerCallback> serverCallback);

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver);
            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver);
            void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver);

        private:
            DataMessage process(const DataMessage& request) const override;

            void onShutdownRequested() override;

            MessageBuilder m_messageBuilder;
            std::shared_ptr<PluginCommunication::IPluginServerCallback> m_serverCallback;
        };

    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
