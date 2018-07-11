/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <cassert>
#include "MessageBuilder.h"
namespace Common
{
    namespace PluginProtocol
    {
        using namespace Common::PluginApi;

        MessageBuilder::MessageBuilder(const std::string &protocolVersion, const std::string &pluginName)
                : m_pluginName(pluginName)
        , m_protocolVersion(protocolVersion)
        {

        }

        DataMessage MessageBuilder::requestSendEventMessage(const std::string &appId, const std::string &eventXml) const
        {
            return createDefaultDataMessage(Commands::PLUGIN_SEND_EVENT, appId, eventXml);
        }

        DataMessage MessageBuilder::requestSendStatusMessage(const std::string &appId, const std::string &statusXml,
                                                             const std::string &statusWithoutTimeStamp) const
        {
            DataMessage dataMessage = createDefaultDataMessage(Commands::PLUGIN_SEND_STATUS, appId, statusXml);
            dataMessage.Payload.push_back(statusWithoutTimeStamp);
            return dataMessage;
        }

        DataMessage MessageBuilder::requestRegisterMessage() const
        {
            return createDefaultDataMessage(Commands::PLUGIN_SEND_REGISTER, std::string(), std::string());
        }

        DataMessage MessageBuilder::requestCurrentPolicyMessage(const std::string &appId) const
        {
            return createDefaultDataMessage(Commands::PLUGIN_QUERY_CURRENT_POLICY, appId, std::string());
        }

        DataMessage
        MessageBuilder::requestApplyPolicyMessage(const std::string &appId, const std::string &policyContent) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_APPLY_POLICY, appId, policyContent);
        }

        DataMessage
        MessageBuilder::requestDoActionMessage(const std::string &appId, const std::string &actionContent) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_DO_ACTION, appId, actionContent);
        }

        DataMessage MessageBuilder::requestRequestPluginStatusMessage(const std::string &appId) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_STATUS, appId, std::string());
        }

        DataMessage MessageBuilder::requestRequestTelemetryMessage() const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_TELEMETRY, std::string(), std::string());
        }

        std::string MessageBuilder::requestExtractEvent(const DataMessage & dataMessage) const
        {
            assert( dataMessage.Command == Commands::PLUGIN_SEND_EVENT);
            return dataMessage.Payload.at(0);
        }

        Common::PluginApi::StatusInfo MessageBuilder::requestExtractStatus(const DataMessage &dataMessage) const
        {
            assert( dataMessage.Command == Commands::PLUGIN_SEND_STATUS || dataMessage.Command == Commands::REQUEST_PLUGIN_STATUS);
            return {dataMessage.Payload.at(0), dataMessage.Payload.at(1), dataMessage.ApplicationId};
        }

        std::string MessageBuilder::requestExtractPolicy(const DataMessage &dataMessage) const
        {
            assert( dataMessage.Command == Commands::REQUEST_PLUGIN_APPLY_POLICY);
            return dataMessage.Payload.at(0);
        }

        std::string MessageBuilder::requestExtractAction(const DataMessage &dataMessage) const
        {
            assert( dataMessage.Command == Commands::REQUEST_PLUGIN_DO_ACTION);
            return dataMessage.Payload.at(0);
        }

        DataMessage MessageBuilder::replyAckMessage(const DataMessage &dataMessage) const
        {
            DataMessage reply(dataMessage);
            reply.Payload.clear();
            reply.Payload.emplace_back("ACK");
            return reply;
        }

        DataMessage MessageBuilder::replySetErrorIfEmpty(const DataMessage &dataMessage, const std::string &errorDescription) const
        {
            DataMessage reply(dataMessage);
            reply.Payload.clear();
            if ( reply.Error.empty())
            {
                reply.Error = errorDescription;
            }

            return reply;

        }

        DataMessage MessageBuilder::replyCurrentPolicy(const DataMessage &dataMessage, const std::string &policyContent) const
        {
            assert(dataMessage.Command == Commands::PLUGIN_QUERY_CURRENT_POLICY);
            DataMessage reply(dataMessage);
            reply.Payload.clear();
            reply.Payload.push_back(policyContent);
            return reply;
        }

        DataMessage MessageBuilder::replyTelemetry(const DataMessage & dataMessage, const std::string &telemetryContent) const
        {
            assert(dataMessage.Command == Commands::REQUEST_PLUGIN_TELEMETRY);
            DataMessage reply(dataMessage);
            reply.Payload.clear();
            reply.Payload.push_back(telemetryContent);
            return reply;
        }

        DataMessage MessageBuilder::replyStatus(const DataMessage & dataMessage, const Common::PluginApi::StatusInfo &statusInfo) const
        {
            assert(dataMessage.Command == Commands::REQUEST_PLUGIN_STATUS);
            DataMessage reply(dataMessage);
            reply.Payload.clear();
            reply.Payload.push_back(statusInfo.statusXml);
            reply.Payload.push_back(statusInfo.statusWithoutXml);
            return reply;
        }

        std::string MessageBuilder::replyExtractCurrentPolicy(const DataMessage &dataMessage) const
        {
            assert(dataMessage.Command == Commands::PLUGIN_QUERY_CURRENT_POLICY);
            return dataMessage.Payload.at(0);
        }

        std::string MessageBuilder::replyExtractTelemetry(const DataMessage &dataMessage) const
        {
            assert(dataMessage.Command == Commands::REQUEST_PLUGIN_TELEMETRY);
            return dataMessage.Payload.at(0);
        }

        DataMessage
        MessageBuilder::createDefaultDataMessage(Common::PluginProtocol::Commands command, const std::string &appId,
                                                 const std::string &payload) const
        {
            DataMessage dataMessage;
            dataMessage.ApplicationId = appId;
            dataMessage.PluginName = m_pluginName;
            dataMessage.Command = command;
            dataMessage.ProtocolVersion = m_protocolVersion;
            if ( !payload.empty())
            {
                dataMessage.Payload.push_back(payload);
            }
            return dataMessage;
        }

        bool MessageBuilder::hasAck(const DataMessage &dataMessage) const
        {
            if (!dataMessage.Payload.empty())
            {
                return dataMessage.Payload[0] == "ACK";
            }

            return false;
        }

    }
}
