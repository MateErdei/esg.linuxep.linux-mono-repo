//
// Created by pair on 04/07/18.
//

#ifndef EVEREST_BASE_IPLUGINAPISERVERCALLBACK_H
#define EVEREST_BASE_IPLUGINAPISERVERCALLBACK_H

#include <string>
#include "Common/PluginApi/IPluginCallback.h"
#include "Common/PluginApi/DataMessage.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginServerCallback
    {
    public:
        virtual ~IPluginServerCallback() = default;
        virtual void receivedSendEvent(const std::string& appId, const std::string &eventXml) = 0;
        virtual void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo) = 0;
        virtual void shutdown() = 0;
        virtual std::string receivedGetPolicy(const std::string &pluginName) = 0;
        virtual void receivedRegisterWithManagementAgent(const Common::PluginApi::RegistrationInfo &regInfo) = 0;
    };
}
}

#endif //EVEREST_BASE_IPLUGINAPISERVERCALLBACK_H
