/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MessageBuilder.h"

#include <cassert>
namespace Common
{
    namespace PluginProtocol
    {
        using namespace Common::PluginApi;

        MessageBuilder::MessageBuilder(const std::string& pluginName) : m_pluginName(pluginName) {}

        DataMessage MessageBuilder::requestSendEventMessage(const std::string& appId, const std::string& eventXml) const
        {
            return createDefaultDataMessage(Commands::PLUGIN_SEND_EVENT, appId, eventXml);
        }

        DataMessage MessageBuilder::requestSendStatusMessage(
            const std::string& appId,
            const std::string& statusXml,
            const std::string& statusWithoutTimeStamp) const
        {
            DataMessage dataMessage = createDefaultDataMessage(Commands::PLUGIN_SEND_STATUS, appId, statusXml);
            dataMessage.m_payload.push_back(statusWithoutTimeStamp);
            return dataMessage;
        }

        DataMessage MessageBuilder::requestRegisterMessage() const
        {
            return createDefaultDataMessage(Commands::PLUGIN_SEND_REGISTER, std::string(), std::string());
        }

        DataMessage MessageBuilder::requestCurrentPolicyMessage(const std::string& appId) const
        {
            return createDefaultDataMessage(Commands::PLUGIN_QUERY_CURRENT_POLICY, appId, std::string());
        }

        DataMessage MessageBuilder::requestApplyPolicyMessage(
            const std::string& appId,
            const std::string& policyContent) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_APPLY_POLICY, appId, policyContent);
        }

        DataMessage MessageBuilder::requestDoActionMessage(const std::string& appId, const std::string& actionContent)
            const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_DO_ACTION, appId, actionContent);
        }

        DataMessage MessageBuilder::requestRequestPluginStatusMessage(const std::string& appId) const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_STATUS, appId, std::string());
        }

        DataMessage MessageBuilder::requestRequestTelemetryMessage() const
        {
            return createDefaultDataMessage(Commands::REQUEST_PLUGIN_TELEMETRY, std::string(), std::string());
        }

        std::string MessageBuilder::requestExtractEvent(const DataMessage& dataMessage) const
        {
            assert(dataMessage.m_command == Commands::PLUGIN_SEND_EVENT);
            return dataMessage.m_payload.at(0);
        }

        Common::PluginApi::StatusInfo MessageBuilder::requestExtractStatus(const DataMessage& dataMessage) const
        {
            assert(
                dataMessage.m_command == Commands::PLUGIN_SEND_STATUS ||
                dataMessage.m_command == Commands::REQUEST_PLUGIN_STATUS);
            return { dataMessage.m_payload.at(0), dataMessage.m_payload.at(1), dataMessage.m_applicationId };
        }

        std::string MessageBuilder::requestExtractPolicy(const DataMessage& dataMessage) const
        {
            assert(dataMessage.m_command == Commands::REQUEST_PLUGIN_APPLY_POLICY);
            return dataMessage.m_payload.at(0);
        }

        std::string MessageBuilder::requestExtractAction(const DataMessage& dataMessage) const
        {
            assert(dataMessage.m_command == Commands::REQUEST_PLUGIN_DO_ACTION);
            return dataMessage.m_payload.at(0);
        }

        DataMessage MessageBuilder::replyAckMessage(const DataMessage& dataMessage) const
        {
            DataMessage reply(dataMessage);
            reply.m_payload.clear();
            reply.m_acknowledge = true;
            return reply;
        }

        DataMessage MessageBuilder::replySetErrorIfEmpty(
            const DataMessage& dataMessage,
            const std::string& errorDescription) const
        {
            DataMessage reply(dataMessage);
            reply.m_payload.clear();
            if (reply.m_error.empty())
            {
                reply.m_error = errorDescription;
            }

            return reply;
        }

        DataMessage MessageBuilder::replyCurrentPolicy(const DataMessage& dataMessage, const std::string& policyContent)
            const
        {
            assert(dataMessage.m_command == Commands::PLUGIN_QUERY_CURRENT_POLICY);
            DataMessage reply(dataMessage);
            reply.m_payload.clear();
            reply.m_payload.push_back(policyContent);
            return reply;
        }

        DataMessage MessageBuilder::replyTelemetry(const DataMessage& dataMessage, const std::string& telemetryContent)
            const
        {
            assert(dataMessage.m_command == Commands::REQUEST_PLUGIN_TELEMETRY);
            DataMessage reply(dataMessage);
            reply.m_payload.clear();
            reply.m_payload.push_back(telemetryContent);
            return reply;
        }

        DataMessage MessageBuilder::replyStatus(
            const DataMessage& dataMessage,
            const Common::PluginApi::StatusInfo& statusInfo) const
        {
            assert(dataMessage.m_command == Commands::REQUEST_PLUGIN_STATUS);
            DataMessage reply(dataMessage);
            reply.m_payload.clear();
            reply.m_payload.push_back(statusInfo.statusXml);
            reply.m_payload.push_back(statusInfo.statusWithoutTimestampsXml);
            return reply;
        }

        std::string MessageBuilder::replyExtractCurrentPolicy(const DataMessage& dataMessage) const
        {
            assert(dataMessage.m_command == Commands::PLUGIN_QUERY_CURRENT_POLICY);
            return dataMessage.m_payload.at(0);
        }

        std::string MessageBuilder::replyExtractTelemetry(const DataMessage& dataMessage) const
        {
            assert(dataMessage.m_command == Commands::REQUEST_PLUGIN_TELEMETRY);
            return dataMessage.m_payload.at(0);
        }

        DataMessage MessageBuilder::createDefaultDataMessage(
            Common::PluginProtocol::Commands command,
            const std::string& appId,
            const std::string& payload) const
        {
            DataMessage dataMessage;
            dataMessage.m_applicationId = appId;
            dataMessage.m_pluginName = m_pluginName;
            dataMessage.m_command = command;
            if (!payload.empty())
            {
                dataMessage.m_payload.push_back(payload);
            }
            return dataMessage;
        }

        bool MessageBuilder::hasAck(const DataMessage& dataMessage) const { return dataMessage.m_acknowledge; }

    } // namespace PluginProtocol
} // namespace Common
