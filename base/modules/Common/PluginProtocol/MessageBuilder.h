/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_REQUESTSBUILDER_H
#define EVEREST_BASE_REQUESTSBUILDER_H

#include <Common/PluginApi/IPluginCallback.h>
#include "DataMessage.h"
namespace Common
{
    namespace PluginProtocol
    {

        class MessageBuilder
        {
        public:
            MessageBuilder( const std::string& applicationID, const std::string &  protocolVersion);

            /** Create the requests as client **/
            DataMessage requestSendEventMessage( const std::string & eventXml) const;
            DataMessage requestSendStatusMessage(const std::string & statusXml, const std::string & statusWithoutTimeStamp) const;
            DataMessage requestRegisterMessage() const;
            DataMessage requestCurrentPolicyMessage() const;
            DataMessage requestApplyPolicyMessage(const std::string & policyContent) const;
            DataMessage requestDoActionMessage( const std::string & actionContent) const;
            DataMessage requestRequestPluginStatusMessage() const;
            DataMessage requestRequestTelemetryMessage() const;


            /** Extracting information from requests as server **/
            std::string requestExtractEvent( const DataMessage & ) const;
            Common::PluginApi::StatusInfo requestExtractStatus( const DataMessage & ) const;
            std::string requestExtractPolicy(const DataMessage & ) const;
            std::string requestExtractAction( const DataMessage & ) const;
            std::string requestExtractPluginName( const DataMessage & ) const;


            /** Build replies as servers **/
            DataMessage replyAckMessage( const DataMessage & ) const;
            DataMessage replySetErrorIfEmpty( const DataMessage& , const std::string & errorDescription )  const;
            DataMessage replyCurrentPolicy(const DataMessage & , const std::string & policyContent) const;
            DataMessage replyTelemetry( const DataMessage &, const std::string & telemetryContent) const;
            DataMessage replyStatus(const DataMessage&, const Common::PluginApi::StatusInfo &) const;


            /** Extracting information from replies as client */
            std::string replyExtractCurrentPolicy( const DataMessage & ) const;
            std::string replyExtractTelemetry( const DataMessage & ) const;

            bool hasAck(const DataMessage &dataMessage) const;

        private:
            DataMessage createDefaultDataMessage(Common::PluginProtocol::Commands command, const std::string& payload) const;


            std::string m_applicationID;
            std::string m_protocolVersion;

        };
    }
}
#endif //EVEREST_BASE_REQUESTSBUILDER_H
