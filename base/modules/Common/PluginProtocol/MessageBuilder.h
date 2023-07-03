/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "DataMessage.h"

#include "modules/Common/PluginApi/StatusInfo.h"

#include "modules/Common/PluginApi/IPluginCallbackApi.h"
namespace Common
{
    namespace PluginProtocol
    {
        class MessageBuilder
        {
        public:
            explicit MessageBuilder(const std::string& pluginName);

            /** Create the requests as client **/
            DataMessage requestSendEventMessage(const std::string& appId, const std::string& eventXml) const;

            DataMessage requestSendStatusMessage(
                const std::string& appId,
                const std::string& statusXml,
                const std::string& statusWithoutTimeStamp) const;
            DataMessage requestRegisterMessage() const;

            DataMessage requestCurrentPolicyMessage(const std::string& appId) const;

            DataMessage requestApplyPolicyMessage(const std::string& appId, const std::string& policyContent) const;

            DataMessage requestDoActionMessage(
                const std::string& appId,
                const std::string& actionContent,
                const std::string& correlationId) const;

            DataMessage requestRequestPluginStatusMessage(const std::string& appId) const;
            DataMessage requestRequestTelemetryMessage() const;
            DataMessage requestRequestHealthMessage() const;
            DataMessage sendThreatHealthMessage(const std::string& threatHealthJson) const;


            /** Extracting information from requests as server **/
            std::string requestExtractEvent(const DataMessage&) const;
            PluginApi::StatusInfo requestExtractStatus(const DataMessage&) const;
            std::string requestExtractPolicy(const DataMessage&) const;
            std::string requestExtractAction(const DataMessage&) const;
            std::string requestExtractThreatHealth(const DataMessage&) const;

            /** Build replies as servers **/
            DataMessage replyAckMessage(const DataMessage&) const;
            DataMessage replySetErrorIfEmpty(const DataMessage&, const std::string& errorDescription) const;
            DataMessage replyCurrentPolicy(const DataMessage&, const std::string& policyContent) const;
            DataMessage replyTelemetry(const DataMessage&, const std::string& telemetryContent) const;
            DataMessage replyStatus(const DataMessage&, const Common::PluginApi::StatusInfo&) const;
            DataMessage replyHealth(const DataMessage&, const std::string& healthContent) const;

            /** Extracting information from replies as client */
            std::string replyExtractCurrentPolicy(const DataMessage&) const;
            std::string replyExtractTelemetry(const DataMessage&) const;
            std::string replyExtractHealth(const DataMessage&) const;

            bool hasAck(const DataMessage& dataMessage) const;

        private:
            DataMessage createDefaultDataMessage(
                Common::PluginProtocol::Commands command,
                const std::string& appId,
                const std::string& payload) const;

            std::string m_pluginName;
        };
    } // namespace PluginProtocol
} // namespace Common
