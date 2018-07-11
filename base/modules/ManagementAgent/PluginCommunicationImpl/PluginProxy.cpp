/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IPluginCommunicationException.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"
#include "Common/PluginProtocol/MessageBuilder.h"
#include "Common/PluginProtocol/Protocol.h"
#include "PluginProxy.h"
#include <algorithm>

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{

    PluginProxy::PluginProxy(Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester, const std::string &pluginName) :
            m_socket(std::move(socketRequester)),
            m_messageBuilder(Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion, pluginName)
    {
        m_appIds.push_back(pluginName);
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

    void PluginProxy::doAction(const std::string &appId, const std::string &actionXml)
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
        for (auto & appId : m_appIds)
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

    void PluginProxy::setAppIds(const std::vector<std::string> &appIds)
    {
        m_appIds = appIds;
    }

    bool PluginProxy::hasAppId(const std::string &appId)
    {
        return (std::find(m_appIds.begin(), m_appIds.end(), appId) != m_appIds.end());
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

        if( reply.Command != request.Command)
        {
            throw PluginCommunication::IPluginCommunicationException(
                    "Received reply from wrong command, expecting" + Common::PluginProtocol::SerializeCommand(request.Command) +
                    ", Received: " + Common::PluginProtocol::SerializeCommand(request.Command));
        }

        if ( !reply.Error.empty())
        {
            throw PluginCommunication::IPluginCommunicationException(reply.Error);
        }

        return reply;

    }
}
}

