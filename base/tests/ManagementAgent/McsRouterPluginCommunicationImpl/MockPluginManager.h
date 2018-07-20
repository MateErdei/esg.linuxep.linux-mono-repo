/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_MOCKPLUGINMANAGER_H
#define MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_MOCKPLUGINMANAGER_H

#include "ManagementAgent/PluginCommunication/IPluginManager.h"

#include <gmock/gmock.h>
#include <string>
#include <vector>


using namespace ::testing;

using ManagementAgent::PluginCommunication::IStatusReceiver;

class MockPluginManager : public ManagementAgent::PluginCommunication::IPluginManager
{
public:
    MOCK_METHOD2(applyNewPolicy, int(const std::string &appId, const std::string &policyXml));
    MOCK_METHOD2(queueAction, int(const std::string &appId, const std::string &actionXml));
    MOCK_METHOD1(getStatus, std::vector<Common::PluginApi::StatusInfo>(const std::string &pluginName));
    MOCK_METHOD1(getTelemetry, std::string (const std::string &pluginName));
    MOCK_METHOD3(setAppIds, void(const std::string &pluginName, const std::vector<std::string> &policyAppIds, const std::vector<std::string> & statusAppIds));
    MOCK_METHOD1(registerPlugin, void(const std::string &pluginName));
    MOCK_METHOD1(removePlugin, void(const std::string &pluginName));
    MOCK_METHOD1(setStatusReceiver, void(std::shared_ptr<IStatusReceiver>& statusReceiver));
public:
};


#endif //MANAGEMENTAGENT_MCSROUTERPLUGINCOMMUNICATIONIMPL_MOCKPLUGINMANAGER_H
