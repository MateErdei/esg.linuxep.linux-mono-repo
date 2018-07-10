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
            m_socket(std::move(socketRequester))
    {
        m_appIds.push_back(pluginName);
    }

    void PluginProxy::applyNewPolicy(const std::string &appId, const std::string &policyXml)
    {
        Common::PluginProtocol::MessageBuilder messageBuilder(appId, Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginProtocol::DataMessage message = messageBuilder.requestApplyPolicyMessage(policyXml);
        Common::PluginProtocol::DataMessage replyMessage = getReply(message);
        if ( !messageBuilder.hasAck(replyMessage))
        {
            throw PluginCommunication::IPluginCommunicationException("Invalid reply for: 'policy event'");
        }
    }

    void PluginProxy::doAction(const std::string &appId, const std::string &actionXml)
    {
        Common::PluginProtocol::MessageBuilder messageBuilder(appId, Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginProtocol::DataMessage message = messageBuilder.requestDoActionMessage(actionXml);
        Common::PluginProtocol::DataMessage replyMessage = getReply(message);
        if ( !messageBuilder.hasAck(replyMessage))
        {
            throw PluginCommunication::IPluginCommunicationException("Invalid reply for: 'action event'");
        }
    }

    std::vector<Common::PluginApi::StatusInfo> PluginProxy::getStatus()
    {
        std::vector<Common::PluginApi::StatusInfo> statusList;
        for (auto & appId : m_appIds)
        {
            Common::PluginProtocol::MessageBuilder messageBuilder(appId,
                                                                  Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion
            );
            Common::PluginProtocol::DataMessage message = messageBuilder.requestRequestPluginStatusMessage();
            Common::PluginProtocol::DataMessage reply = getReply(message);
            statusList.emplace_back(messageBuilder.requestExtractStatus(reply));
        }

        return statusList;
    }

    std::string PluginProxy::getTelemetry()
    {
        Common::PluginProtocol::MessageBuilder messageBuilder("Telemetry request", Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginProtocol::DataMessage message = messageBuilder.requestRequestTelemetryMessage();
        Common::PluginProtocol::DataMessage reply = getReply(message);

        return messageBuilder.replyExtractTelemetry(reply);
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

