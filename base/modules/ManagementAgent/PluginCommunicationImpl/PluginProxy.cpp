/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IPluginCommunicationException.h"
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/PluginProtocol/Protocol.h>
#include "PluginProxy.h"
#include <algorithm>

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{

    PluginProxy::PluginProxy(Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester, const std::string &pluginName) :
            m_socket(std::move(socketRequester)),
            m_messageBuilder(pluginName),
            m_name(pluginName)
    {
        m_appIdCollection.setAppIdsForStatus({pluginName});
        m_appIdCollection.setAppIdsForPolicyAndActions({pluginName});
    }

    void PluginProxy::applyNewPolicy(const std::string &appId, const std::string &policyXml)
    {
        Common::PluginProtocol::DataMessage replyMessage = getReply(
                m_messageBuilder.requestApplyPolicyMessage(appId, policyXml)
        );
        if (!m_messageBuilder.hasAck(replyMessage))
        {
            throw PluginCommunication::IPluginCommunicationException("Invalid reply for: 'policy event'");
        }
    }

    void PluginProxy::queueAction(const std::string &appId, const std::string &actionXml)
    {
        Common::PluginProtocol::DataMessage replyMessage = getReply(
                m_messageBuilder.requestDoActionMessage(appId, actionXml)
        );
        if (!m_messageBuilder.hasAck(replyMessage))
        {
            throw PluginCommunication::IPluginCommunicationException("Invalid reply for: 'action event'");
        }
    }

    std::vector<Common::PluginApi::StatusInfo> PluginProxy::getStatus()
    {
        std::vector<Common::PluginApi::StatusInfo> statusList;
        for (auto & appId : m_appIdCollection.statusAppIds())
        {
            Common::PluginProtocol::DataMessage reply = getReply(
                    m_messageBuilder.requestRequestPluginStatusMessage(appId)
            );
            statusList.emplace_back(m_messageBuilder.requestExtractStatus(reply));
        }

        return statusList;
    }

    std::string PluginProxy::getTelemetry()
    {
        Common::PluginProtocol::DataMessage reply = getReply(
                m_messageBuilder.requestRequestTelemetryMessage()
        );

        return m_messageBuilder.replyExtractTelemetry(reply);
    }



    Common::PluginProtocol::DataMessage PluginProxy::getReply(const Common::PluginProtocol::DataMessage &request) const
    {
        Common::PluginProtocol::Protocol protocol;
        Common::PluginProtocol::DataMessage reply;
        try
        {
            m_socket->write(protocol.serialize(request));
            reply = protocol.deserialize(m_socket->read());
        }
        catch ( std::exception & ex)
        {
            throw PluginCommunication::IPluginCommunicationException(ex.what());
        }

        if( reply.m_command != request.m_command)
        {
            throw PluginCommunication::IPluginCommunicationException(
                    "Received reply from wrong command, expecting" +
                            Common::PluginProtocol::ConvertCommandEnumToString(request.m_command) +
                    ", Received: " + Common::PluginProtocol::ConvertCommandEnumToString(request.m_command));
        }

        if ( !reply.m_error.empty())
        {
            throw PluginCommunication::IPluginCommunicationException(reply.m_error);
        }

        return reply;

    }

    void PluginProxy::setPolicyAndActionsAppIds(const std::vector<std::string> &appIds)
    {
        m_appIdCollection.setAppIdsForPolicyAndActions(appIds);
    }

    void PluginProxy::setStatusAppIds(const std::vector<std::string> &appIds)
    {
        m_appIdCollection.setAppIdsForStatus(appIds);

    }

    bool PluginProxy::hasPolicyAppId(const std::string &appId)
    {
        return m_appIdCollection.usePolicyId(appId);
    }

    bool PluginProxy::hasActionAppId(const std::string &appId)
    {
        return m_appIdCollection.implementActionId(appId);
    }

    bool PluginProxy::hasStatusAppId(const std::string &appId)
    {
        return m_appIdCollection.implementStatus(appId);
    }
}
}

