//
// Created by pair on 05/07/18.
//

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
        PluginProxy(Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester, std::vector<std::string> appIds);

        void applyNewPolicy(const std::string &appId, const std::string &policyXml) override;

        void doAction(const std::string &appId, const std::string &actionXml) override;

        Common::PluginApi::StatusInfo getStatus(void) override;

        std::string getTelemetry() override;

        bool hasAppId(const std::string &appId) override;

    private:

        Common::PluginApiImpl::DataMessage getReply(const Common::PluginApiImpl::DataMessage &request) const;
        Common::ZeroMQWrapper::ISocketRequesterPtr  m_socket;
        std::vector<std::string> m_AppIds;
    };

}
}


#endif //EVEREST_BASE_PLUGINPROXY_H
