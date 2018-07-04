/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <cassert>
#include "MessageBuilder.h"
namespace Common
{
    namespace PluginApiImpl
    {
        using namespace Common::PluginApi;
        MessageBuilder::MessageBuilder(const std::string &applicationID, const std::string &protocolVersion)
        : m_applicationID(applicationID)
        , m_protocolVersion(protocolVersion)
        {

        }

        DataMessage MessageBuilder::requestSendEventMessage(const std::string &eventXml) const
        {
            return createDefaultDataMessage(Commands::PLUGIN_SEND_EVENT, eventXml);
        }

        DataMessage MessageBuilder::requestSendStatusMessage(const std::string &statusXml,
                                                             const std::string &statusWithoutTimeStamp) const
        {
            DataMessage dataMessage = createDefaultDataMessage(Commands::PLUGIN_SEND_STATUS, statusXml);
            dataMessage.payload.push_back(statusWithoutTimeStamp);
            return dataMessage;
        }

        DataMessage MessageBuilder::requestRegisterMessage() const
        {
            return createDefaultDataMessage(Commands::PLUGIN_SEND_REGISTER, ""); // TODO check payload ?
        }

        DataMessage MessageBuilder::requestCurrentPolicyMessage() const
        {
            return createDefaultDataMessage(Commands::PLUGIN_QUERY_CURRENT_POLICY, ""); // TODO check payload ?
        }

        DataMessage MessageBuilder::requestApplyPolicyMessage(const std::string &policyContent) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_APPLY_POLICY, policyContent);
        }

        DataMessage MessageBuilder::requestDoActionMessage(const std::string &actionContent) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_DO_ACTION, actionContent);
        }

        DataMessage MessageBuilder::requestRequestPluginStatusMessage() const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_STATUS, ""); // TODO check payload ?
        }

        DataMessage MessageBuilder::requestRequestTelemetryMessage() const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_TELEMETRY, ""); // TODO check payload ?
        }

        std::string MessageBuilder::requestExtractEvent(const DataMessage & dataMessage) const
        {
            assert( dataMessage.Command == Commands::PLUGIN_SEND_EVENT);
            return dataMessage.payload.at(0);
        }

        StatusPair MessageBuilder::requestExtractStatus(const DataMessage &dataMessage) const
        {
            assert( dataMessage.Command == Commands::REQUEST_PLUGIN_STATUS);
            return {dataMessage.payload.at(0), dataMessage.payload.at(1)};
        }

        std::string MessageBuilder::requestExtractPolicy(const DataMessage &dataMessage) const
        {
            assert( dataMessage.Command == Commands::REQUEST_PLUGIN_APPLY_POLICY);
            return dataMessage.payload.at(0);
        }

        std::string MessageBuilder::requestExtractAction(const DataMessage &dataMessage) const
        {
            assert( dataMessage.Command == Commands::REQUEST_PLUGIN_DO_ACTION);
            return dataMessage.payload.at(0);
        }

        DataMessage MessageBuilder::replyAckMessage(const DataMessage &dataMessage) const
        {
            DataMessage reply(dataMessage);
            reply.payload.clear();
            reply.payload.push_back("ACK");//?
            return reply;
        }

        DataMessage MessageBuilder::replyErrorMessage(const DataMessage &dataMessage, const std::string &errorDescription) const
        {
            DataMessage reply(dataMessage);
            reply.payload.clear();
            reply.Error = errorDescription;
            return reply;

        }

        DataMessage MessageBuilder::replyCurrentPolicy(const DataMessage &dataMessage, const std::string &policyContent) const
        {
            assert(dataMessage.Command == Commands::PLUGIN_QUERY_CURRENT_POLICY);
            DataMessage reply(dataMessage);
            reply.payload.clear();
            reply.payload.push_back(policyContent);
            return reply;
        }

        DataMessage MessageBuilder::replyTelemetry(const DataMessage & dataMessage, const std::string &telemetryContent) const
        {
            assert(dataMessage.Command == Commands::REQUEST_PLUGIN_TELEMETRY);
            DataMessage reply(dataMessage);
            reply.payload.clear();
            reply.payload.push_back(telemetryContent);
            return reply;
        }

        DataMessage MessageBuilder::replyStatus(const DataMessage & dataMessage, const StatusPair &statusPair) const
        {
            assert(dataMessage.Command == Commands::REQUEST_PLUGIN_STATUS);
            DataMessage reply(dataMessage);
            reply.payload.clear();
            reply.payload.push_back(statusPair.status);
            reply.payload.push_back(statusPair.statusWithoutTimeStamp);
            return reply;
        }

        std::string MessageBuilder::replyExtractCurrentPolicy(const DataMessage &dataMessage) const
        {
            assert(dataMessage.Command == Commands::PLUGIN_QUERY_CURRENT_POLICY);
            return dataMessage.payload.at(0);
        }

        std::string MessageBuilder::replyExtractTelemetry(const DataMessage &dataMessage) const
        {
            assert(dataMessage.Command == Commands::REQUEST_PLUGIN_TELEMETRY);
            return dataMessage.payload.at(0);
        }

        DataMessage
        MessageBuilder::createDefaultDataMessage(Common::PluginApi::Commands command, const std::string &payload) const
        {
            DataMessage dataMessage;
            dataMessage.ApplicationId = m_applicationID;
            dataMessage.Command = command;
            dataMessage.ProtocolVersion = m_protocolVersion;
            dataMessage.payload.push_back(payload);
            return dataMessage;
        }

    }
}
