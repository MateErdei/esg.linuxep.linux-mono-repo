// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ManagementAgent/EventReceiver/IEventReceiver.h"
#include "ManagementAgent/PluginCommunication/IPluginServerCallback.h"
#include "ManagementAgent/PluginCommunication/IPolicyReceiver.h"
#include "ManagementAgent/PluginCommunication/IStatusReceiver.h"

#include <memory>

namespace ManagementAgent::PluginCommunicationImpl
{
    class ISettablePluginServerCallback : virtual public PluginCommunication::IPluginServerCallback
    {
    public:
        using StatusReceiverPtr = std::shared_ptr<PluginCommunication::IStatusReceiver>;
        virtual void setStatusReceiver(StatusReceiverPtr& statusReceiver) = 0;
        virtual void setEventReceiver(PluginCommunication::IEventReceiverPtr& receiver) = 0;
        virtual void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver) = 0;
        virtual void setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver) = 0;
    };
} // namespace ManagementAgent::PluginCommunicationImpl