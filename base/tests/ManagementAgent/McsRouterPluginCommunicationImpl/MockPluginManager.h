// Copyright 2018-2023 Sophos Limited. All rights reserved.

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
    MOCK_METHOD(int, applyNewPolicy, (const std::string& appId, const std::string& policyXml, const std::string& pluginName));
    MOCK_METHOD(int, queueAction, (const std::string& appId, const std::string& actionXml, const std::string& correlationId));
    MOCK_METHOD(std::vector<Common::PluginApi::StatusInfo>, getStatus, (const std::string& pluginName));
    MOCK_METHOD(std::string, getTelemetry, (const std::string& pluginName));
    MOCK_METHOD(bool, checkIfSinglePluginInRegistry, (const std::string& pluginName));
    MOCK_METHOD(std::string, getHealth, (const std::string& pluginName));
    MOCK_METHOD(void, registerAndConfigure,
        (const std::string& pluginName, const ManagementAgent::PluginCommunication::PluginDetails& pluginDetails));
    MOCK_METHOD(void, registerPlugin, (const std::string& pluginName));
    MOCK_METHOD(void, removePlugin, (const std::string& pluginName));
    MOCK_METHOD(std::vector<std::string>, getRegisteredPluginNames, ());
    MOCK_METHOD(void, setStatusReceiver, (std::shared_ptr<IStatusReceiver>& statusReceiver));
    MOCK_METHOD(void, setEventReceiver,
        (std::shared_ptr<ManagementAgent::PluginCommunication::IEventReceiver> receiver));
    MOCK_METHOD(void, setPolicyReceiver,
        (std::shared_ptr<ManagementAgent::PluginCommunication::IPolicyReceiver>& receiver));
    MOCK_METHOD(void, setThreatHealthReceiver,
        (std::shared_ptr<ManagementAgent::PluginCommunication::IThreatHealthReceiver>& receiver));
    MOCK_METHOD(ManagementAgent::PluginCommunication::PluginHealthStatus, getHealthStatusForPlugin,
        (const std::string& pluginName, bool prevHealthMissing));
    MOCK_METHOD(std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus>, getSharedHealthStatusObj, ());
    MOCK_METHOD(bool, updateOngoingWithGracePeriod, (unsigned int gracePeriodSeconds, timepoint_t now));
};
