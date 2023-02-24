// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ManagementAgent/PluginCommunicationImpl/ISettablePluginServerCallback.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"


using namespace ::testing;

namespace
{
    using namespace ManagementAgent::PluginCommunicationImpl;
    using namespace ManagementAgent;
    class MockPluginServerCallback : public ISettablePluginServerCallback
    {
    public:
        MOCK_METHOD2(receivedSendEvent, void(const std::string&, const std::string&));
        MOCK_METHOD(void, handleAction, (const std::string& actionXml), (override));
        MOCK_METHOD(bool, outbreakMode, (), (const, override));
        
        MOCK_METHOD2(receivedChangeStatus, void(const std::string&, const Common::PluginApi::StatusInfo&));
        MOCK_METHOD0(shutdown, void());

        MOCK_METHOD1(receivedGetPolicyRequest, bool(const std::string& appId));
        MOCK_METHOD1(receivedRegisterWithManagementAgent, void(const std::string& pluginName));
        MOCK_METHOD3(
            receivedThreatHealth,
            bool(
                const std::string& pluginName,
                const std::string& threatHealth,
                std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj));

        MOCK_METHOD(
            void,
            setStatusReceiver,
            (ISettablePluginServerCallback::StatusReceiverPtr& receiver),
            (override));

        MOCK_METHOD(
            void,
            setEventReceiver,
            (ManagementAgent::PluginCommunication::IEventReceiverPtr& receiver),
            (override));

        MOCK_METHOD(
            void,
            setPolicyReceiver,
            (std::shared_ptr<ManagementAgent::PluginCommunication::IPolicyReceiver>& receiver),
            (override));

        MOCK_METHOD(
            void,
            setThreatHealthReceiver,
            (std::shared_ptr<ManagementAgent::PluginCommunication::IThreatHealthReceiver>& receiver),
            (override));
    };
}
