/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ManagementAgent/PluginCommunication/IPluginManager.h"

#include <gmock/gmock.h>

#include <string>
#include <vector>

using namespace ::testing;

using ManagementAgent::PluginCommunication::IStatusReceiver;

class MockPluginManager : public ManagementAgent::PluginCommunication::IPluginManager
{
public:
    MOCK_METHOD2(applyNewPolicy, int(const std::string& appId, const std::string& policyXml));
    MOCK_METHOD3(queueAction, int(const std::string& appId, const std::string& actionXml, const std::string& correlationId));
    MOCK_METHOD1(getStatus, std::vector<Common::PluginApi::StatusInfo>(const std::string& pluginName));
    MOCK_METHOD1(getTelemetry, std::string(const std::string& pluginName));
    MOCK_METHOD1(getHealth, std::string(const std::string& pluginName));
    MOCK_METHOD2(
        registerAndConfigure,
        void(
            const std::string& pluginName,
            const ManagementAgent::PluginCommunication::PluginDetails& pluginDetails));
    MOCK_METHOD1(registerPlugin, void(const std::string& pluginName));
    MOCK_METHOD1(removePlugin, void(const std::string& pluginName));
    MOCK_METHOD0(getRegisteredPluginNames, std::vector<std::string>(void));
    MOCK_METHOD1(setStatusReceiver, void(std::shared_ptr<IStatusReceiver>& statusReceiver));
    MOCK_METHOD1(
        setEventReceiver,
        void(std::shared_ptr<ManagementAgent::PluginCommunication::IEventReceiver>& receiver));
    MOCK_METHOD1(
        setPolicyReceiver,
        void(std::shared_ptr<ManagementAgent::PluginCommunication::IPolicyReceiver>& receiver));
    MOCK_METHOD1(getHealthStatusForPlugin, ManagementAgent::PluginCommunication::PluginHealthStatus(const std::string& pluginName));
    MOCK_METHOD0(getSharedHealthStatusObj, std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus>());

public:
};
