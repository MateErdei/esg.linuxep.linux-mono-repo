/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_REQUESTSBUILDER_H
#define EVEREST_BASE_REQUESTSBUILDER_H

#include "Common/PluginApi/DataMessage.h"
#include "Common/PluginApi/IPluginCallback.h"

namespace Common
{
    namespace PluginApiImpl
    {
        using DataMessage = Common::PluginApi::DataMessage;

        class MessageBuilder
        {
        public:
            MessageBuilder( const std::string& applicationID, const std::string &  protocolVersion);

            /** Create the requests as client **/
            //Plugin
            DataMessage requestSendEventMessage( const std::string & eventXml) const;
            DataMessage requestSendStatusMessage(const std::string & statusXml, const std::string & statusWithoutTimeStamp) const;
            DataMessage requestRegisterMessage() const;
            DataMessage requestCurrentPolicyMessage() const;
            //Management Agent
            DataMessage requestApplyPolicyMessage(const std::string & policyContent) const;
            DataMessage requestDoActionMessage( const std::string & actionContent) const;
            DataMessage requestRequestPluginStatusMessage() const;
            DataMessage requestRequestTelemetryMessage() const;


            /** Extracting information from requests as server **/
            //Management Agent
            std::string requestExtractEvent( const DataMessage & ) const;
            PluginApi::StatusInfo requestExtractStatus( const DataMessage & ) const;
            std::string requestExtractCurrentPolicy( const DataMessage &) const;
            std::string requestExtractRegistration(const DataMessage &) const;
            std::string requestExtractPluginName(const DataMessage &) const;
            //Plugin
            std::string requestExtractPolicy(const DataMessage & ) const;
            std::string requestExtractAction( const DataMessage & ) const;




            /** Build replies as servers **/
            //Management Agent and Plugin
            DataMessage replyAckMessage( const DataMessage & ) const;
            DataMessage replySetErrorIfEmpty( const DataMessage& , const std::string & errorDescription )  const;
            //Management Agent
            DataMessage replyCurrentPolicy(const DataMessage & , const std::string & policyContent) const;
            //Plugin
            DataMessage replyTelemetry( const DataMessage &, const std::string & telemetryContent) const;
            DataMessage replyStatus(const DataMessage&, const Common::PluginApi::StatusInfo &) const;


            /** Extracting information from replies as client */
            //Management Agent
            std::string replyExtractTelemetry( const DataMessage & ) const;
            Common::PluginApi::StatusInfo replyExtractStatus( const DataMessage & ) const;
            //Plugin
            std::string replyExtractCurrentPolicy( const DataMessage & ) const;

            bool hasAck(const DataMessage &dataMessage) const;

        private:
            DataMessage createDefaultDataMessage(Common::PluginApi::Commands command, const std::string& payload) const;


            std::string m_applicationID;
            std::string m_protocolVersion;

        };
    }
}
#endif //EVEREST_BASE_REQUESTSBUILDER_H
