/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include "ZeroMQWrapper/IReadWrite.h"
#include "ZeroMQWrapper/ISocketReplier.h"
#include "PluginProtocol/AbstractListenerServer.h"
#include "IPluginServerCallback.h"
#include "PluginProtocol/MessageBuilder.h"

using namespace Common::PluginProtocol;

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{

    class PluginServerCallbackHandler : public AbstractListenerServer
    {
    public:
        PluginServerCallbackHandler(std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
                                    std::shared_ptr<PluginCommunication::IPluginServerCallback> serverCallback);

        void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver);
        void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver);

    private:
        DataMessage process(const DataMessage &request) const override;

        void onShutdownRequested() override;

        MessageBuilder m_messageBuilder;
        std::shared_ptr<PluginCommunication::IPluginServerCallback> m_serverCallback;

    };

}
}




