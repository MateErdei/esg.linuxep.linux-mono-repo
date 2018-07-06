/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef EVEREST_BASE_PLUGINPROXY_H
#define EVEREST_BASE_PLUGINPROXY_H


#include "Common/ZeroMQWrapper/IReadWrite.h"
#include "IPluginProxy.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{

    class PluginProxy : virtual public PluginCommunication::IPluginProxy
    {

    public:
        PluginProxy(Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester, const std::string &pluginName);

        void applyNewPolicy(const std::string &appId, const std::string &policyXml) override;

        void doAction(const std::string &appId, const std::string &actionXml) override;

        Common::PluginProtocol::StatusInfo getStatus() override;

        std::string getTelemetry() override;

        void setAppIds(const std::vector<std::string> &appIds) override;

        bool hasAppId(const std::string &appId) override;

    private:

        Common::PluginProtocol::DataMessage getReply(const Common::PluginProtocol::DataMessage &request) const;
        Common::ZeroMQWrapper::ISocketRequesterPtr  m_socket;
        std::vector<std::string> m_appIds;
    };

}
}


#endif //EVEREST_BASE_PLUGINPROXY_H
