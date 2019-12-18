/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializer.h"

#include "Logger.h"
#include "ProtocolSerializerFactory.h"

#include "Common/PluginApi/ApiException.h"

#include <PluginAPIMessage.pb.h>

namespace Common
{
    namespace PluginProtocol
    {
        PluginProtocolProto::PluginAPIMessage_CommandOption serializeCommand(Commands command)
        {
            switch (command)
            {
                case Commands::PLUGIN_SEND_EVENT:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_SendEvent;
                case Commands::PLUGIN_SEND_STATUS:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_SendStatus;
                case Commands::PLUGIN_SEND_REGISTER:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_Registration;
                case Commands::REQUEST_PLUGIN_APPLY_POLICY:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_ApplyPolicy;
                case Commands::REQUEST_PLUGIN_DO_ACTION:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_DoAction;
                case Commands::REQUEST_PLUGIN_STATUS:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_RequestStatus;
                case Commands::REQUEST_PLUGIN_TELEMETRY:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_Telemetry;
                case Commands::PLUGIN_QUERY_CURRENT_POLICY:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_RequestCurrentPolicy;
                case Commands::UNKNOWN:
                default:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::
                        PluginAPIMessage_CommandOption_InvalidCommand;
            }
        }

        Commands DeserializeCommand(const PluginProtocolProto::PluginAPIMessage_CommandOption& commandOption)
        {
            if (commandOption == serializeCommand(Commands::PLUGIN_SEND_EVENT))
            {
                return Commands::PLUGIN_SEND_EVENT;
            }
            else if (commandOption == serializeCommand(Commands::PLUGIN_SEND_STATUS))
            {
                return Commands::PLUGIN_SEND_STATUS;
            }
            else if (commandOption == serializeCommand(Commands::PLUGIN_SEND_REGISTER))
            {
                return Commands::PLUGIN_SEND_REGISTER;
            }
            else if (commandOption == serializeCommand(Commands::REQUEST_PLUGIN_APPLY_POLICY))
            {
                return Commands::REQUEST_PLUGIN_APPLY_POLICY;
            }
            else if (commandOption == serializeCommand(Commands::REQUEST_PLUGIN_DO_ACTION))
            {
                return Commands::REQUEST_PLUGIN_DO_ACTION;
            }
            else if (commandOption == serializeCommand(Commands::REQUEST_PLUGIN_STATUS))
            {
                return Commands::REQUEST_PLUGIN_STATUS;
            }
            else if (commandOption == serializeCommand(Commands::REQUEST_PLUGIN_TELEMETRY))
            {
                return Commands::REQUEST_PLUGIN_TELEMETRY;
            }
            else if (commandOption == serializeCommand(Commands::PLUGIN_QUERY_CURRENT_POLICY))
            {
                return Commands::PLUGIN_QUERY_CURRENT_POLICY;
            }
            return Commands::UNKNOWN;
        }

        const data_t ProtocolSerializer::serialize(const Common::PluginProtocol::DataMessage& dataMessage) const
        {
            PluginProtocolProto::PluginAPIMessage pluginAPIMessage;

            // Check minimum fields are set before creating message
            std::stringstream missingFields;
            if (dataMessage.m_pluginName.empty())
            {
                missingFields << "PluginName ";
            }
            if (dataMessage.m_command == PluginProtocol::Commands::UNSET)
            {
                missingFields << "Command ";
            }
            if (!missingFields.str().empty())
            {
                std::stringstream errorMessage;
                errorMessage << "Missing required fields for protobuf message: " << missingFields.str();
                LOGERROR("Protocol Serializer error - serialize: " << errorMessage.str());
                throw Common::PluginApi::ApiException(errorMessage.str());
            }

            pluginAPIMessage.set_applicationid(dataMessage.m_applicationId);
            pluginAPIMessage.set_pluginname(dataMessage.m_pluginName);
            pluginAPIMessage.set_command(PluginProtocol::serializeCommand(dataMessage.m_command));

            if ( !dataMessage.m_correlationId.empty())
            {
                pluginAPIMessage.set_correlationid(dataMessage.m_correlationId);
            }

            if (!dataMessage.m_error.empty())
            {
                pluginAPIMessage.set_error(dataMessage.m_error);
            }
            else if (dataMessage.m_acknowledge)
            {
                pluginAPIMessage.set_acknowledge(dataMessage.m_acknowledge);
            }
            if (!dataMessage.m_payload.empty())
            {
                for (auto& message : dataMessage.m_payload)
                {
                    pluginAPIMessage.add_payload(message);
                }
            }
            data_t returnData;
            returnData.push_back(pluginAPIMessage.SerializeAsString());
            return returnData;
        }

        const Common::PluginProtocol::DataMessage ProtocolSerializer::deserialize(const data_t& serializedData)
        {
            Common::PluginProtocol::DataMessage message;
            PluginProtocolProto::PluginAPIMessage deserializedData;

            if (serializedData.empty() || !deserializedData.ParsePartialFromString(serializedData[0]))
            {
                std::string errorMessage = "Bad formed message: Protobuf parse error";
                LOGERROR("Protocol Serializer error - deserialize: " << errorMessage);
                throw Common::PluginApi::ApiException(errorMessage);
            }

            // Check minimum fields are defined post deserialisation
            std::stringstream missingFields;
            if (!deserializedData.has_pluginname())
            {
                missingFields << "PluginName ";
            }
            if (!deserializedData.has_command())
            {
                missingFields << "Command ";
            }
            if (!missingFields.str().empty())
            {
                std::stringstream errorMessage;
                errorMessage << "Bad formed message, missing required fields : " << missingFields.str();
                message.m_error = errorMessage.str();
                LOGERROR("Protocol Serializer error - deserialize: " << message.m_error);
                return message;
            }

            message.m_applicationId = deserializedData.applicationid();
            message.m_pluginName = deserializedData.pluginname();
            message.m_command = PluginProtocol::DeserializeCommand(deserializedData.command());

            if (deserializedData.has_error())
            {
                message.m_error = deserializedData.error();
                LOGSUPPORT("Protocol Serializer error - deserialize: message error: " << deserializedData.error());
            }
            else if (message.m_command == PluginProtocol::Commands::UNKNOWN)
            {
                message.m_error = "Invalid request";
                LOGERROR("Protocol Serializer error - deserialize: " << message.m_error);
            }
            else if (deserializedData.has_acknowledge())
            {
                message.m_acknowledge = true;
            }
            for (auto& payload_content : deserializedData.payload())
            {
                message.m_payload.push_back(payload_content);
            }

            if ( deserializedData.has_correlationid())
            {
                message.m_correlationId = deserializedData.correlationid();
            }

            return message;
        }
    } // namespace PluginProtocol

} // namespace Common
