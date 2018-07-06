/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPLUGINMANAGER_H
#define EVEREST_BASE_IPLUGINMANAGER_H

#include <string>
#include "IPluginProxy.h"
#include "IPluginServerCallback.h"
#include "Common/PluginProtocol/DataMessage.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginManager
    {

        virtual void setDefaultTimeout(int timeoutMs) = 0;
        virtual void setDefaultConnectTimeout(int timeoutMs) = 0;

        virtual void setServerCallback(std::shared_ptr<PluginCommunication::IPluginServerCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replierPtr) = 0;


        virtual void applyNewPolicy(const std::string &appId, const std::string &policyXml) = 0;
        virtual void doAction(const std::string &appId, const std::string &actionXml) = 0;
        virtual Common::PluginProtocol::StatusInfo getStatus(const std::string &pluginName) = 0;
        virtual std::string getTelemetry(const std::string &pluginName) = 0;
        virtual void setAppIds(const std::string& pluginName, const std::vector<std::string> &appIds) = 0;

        virtual void registerPlugin(const std::string &pluginName) = 0;
        virtual void removePlugin(const std::string &pluginName) = 0;
    };
}
}

#endif //EVEREST_BASE_IPLUGINMANAGER_H

