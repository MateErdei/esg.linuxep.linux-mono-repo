//
// Created by pair on 05/07/18.
//

#include "IPluginCommunicationException.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"
#include "Common/PluginApiImpl/MessageBuilder.h"
#include "Common/PluginApiImpl/Protocol.h"
#include "PluginProxy.h"

namespace ManagementAgent
{
namespace PluginCommunicationImpl
{

    PluginProxy::PluginProxy(Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester) :
            m_socket(std::move(socketRequester))
    {}


    void PluginProxy::applyNewPolicy(const std::string &appId, const std::string &policyXml)
    {
        Common::PluginApiImpl::MessageBuilder messageBuilder(appId, Common::PluginApiImpl::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginApiImpl::DataMessage message = messageBuilder.requestApplyPolicyMessage(policyXml);
        Common::PluginApiImpl::DataMessage replyMessage = getReply(message);
        if ( !messageBuilder.hasAck(replyMessage))
        {
            throw PluginCommunication::IPluginCommunicationException("Invalid reply for: 'policy event'");
        }
    }

    void PluginProxy::doAction(const std::string &appId, const std::string &actionXml)
    {
        Common::PluginApiImpl::MessageBuilder messageBuilder(appId, Common::PluginApiImpl::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginApiImpl::DataMessage message = messageBuilder.requestDoActionMessage(actionXml);
        Common::PluginApiImpl::DataMessage replyMessage = getReply(message);
        if ( !messageBuilder.hasAck(replyMessage))
        {
            throw PluginCommunication::IPluginCommunicationException("Invalid reply for: 'action event'");
        }
    }

    Common::PluginApi::StatusInfo PluginProxy::getStatus()
    {
        Common::PluginApiImpl::MessageBuilder messageBuilder("Status request", Common::PluginApiImpl::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginApiImpl::DataMessage message = messageBuilder.requestRequestPluginStatusMessage();
        Common::PluginApiImpl::DataMessage reply = getReply(message);

        return messageBuilder.replyExtractStatus(reply);
    }

    std::string PluginProxy::getTelemetry()
    {
        Common::PluginApiImpl::MessageBuilder messageBuilder("Telemetry request", Common::PluginApiImpl::ProtocolSerializerFactory::ProtocolVersion);
        Common::PluginApiImpl::DataMessage message = messageBuilder.requestRequestTelemetryMessage();
        Common::PluginApiImpl::DataMessage reply = getReply(message);

        return messageBuilder.replyExtractTelemetry(reply);
    }

    Common::PluginApiImpl::DataMessage PluginProxy::getReply(const Common::PluginApiImpl::DataMessage &request) const
    {
        Common::PluginApiImpl::Protocol protocol;
        Common::PluginApiImpl::DataMessage reply;
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
                    "Received reply from wrong command, expecting" + Common::PluginApi::SerializeCommand(request.Command) +
                    ", Received: " + Common::PluginApi::SerializeCommand(request.Command));
        }

        if ( !reply.Error.empty())
        {
            throw PluginCommunication::IPluginCommunicationException(reply.Error);
        }

        return reply;

    }
}
}

