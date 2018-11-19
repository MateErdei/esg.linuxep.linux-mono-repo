/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProtocolSerializer.h"
#include "ProtocolSerializerFactory.h"
#include "Logger.h"
#include <PluginAPIMessage.pb.h>
#include "Common/PluginApi/ApiException.h"

namespace Common
{
    namespace PluginProtocol
    {
        PluginProtocolProto::PluginAPIMessage_CommandOption serializeCommand(Commands command)
        {
            switch (command)
            {
                case Commands::PLUGIN_SEND_EVENT:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_SendEvent;
                case Commands::PLUGIN_SEND_STATUS:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_SendStatus;
                case Commands::PLUGIN_SEND_REGISTER:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_Registration;
                case Commands::REQUEST_PLUGIN_APPLY_POLICY:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_ApplyPolicy;
                case Commands::REQUEST_PLUGIN_DO_ACTION:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_DoAction;
                case Commands::REQUEST_PLUGIN_STATUS:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_RequestStatus;
                case Commands::REQUEST_PLUGIN_TELEMETRY:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_Telemetry;
                case Commands::PLUGIN_QUERY_CURRENT_POLICY:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_RequestCurrentPolicy;
                case Commands::UNKNOWN:
                default:
                    return PluginProtocolProto::PluginAPIMessage_CommandOption::PluginAPIMessage_CommandOption_InvalidCommand;

            }
        }

        Commands DeserializeCommand(const PluginProtocolProto::PluginAPIMessage_CommandOption &commandOption)
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


        const data_t ProtocolSerializer::serialize(const Common::PluginProtocol::DataMessage &dataMessage)const
        {
            PluginProtocolProto::PluginAPIMessage pluginAPIMessage;

            pluginAPIMessage.set_applicationid(dataMessage.ApplicationId);
            pluginAPIMessage.set_pluginname(dataMessage.PluginName);
            pluginAPIMessage.set_command(PluginProtocol::serializeCommand(dataMessage.Command));
            pluginAPIMessage.set_messageid(dataMessage.MessageId);
            pluginAPIMessage.add_payload();

            if (!dataMessage.Error.empty())
            {
                pluginAPIMessage.set_error(dataMessage.Error);
            }
            else if (!dataMessage.Payload.empty()){
                for (auto &message: dataMessage.Payload) {
                    pluginAPIMessage.add_payload(message);
                }
            }
            std::string output;
            if (!pluginAPIMessage.SerializeToString(&output)) {
                std::string errorMessage = "Protocol Serializer error: Failed to serialize message";
                LOGERROR(errorMessage);
                throw Common::PluginApi::ApiException("Protocol Serializer error: Failed to serialize message");
            }
            data_t returnData;
            returnData.push_back(output);
            return returnData;
        }

        const Common::PluginProtocol::DataMessage ProtocolSerializer::deserialize(const data_t &serializedData)
        {
            Common::PluginProtocol::DataMessage message;
            PluginProtocolProto::PluginAPIMessage deserializedData;

            if (serializedData.empty() || deserializedData.ParseFromString(serializedData[0]))
            {
                message = createDefaultErrorMessage();
                message.Error = "Bad formed message: Protobuf parse error";
                LOGERROR("Protocol Serializer error: " << message.Error);
                return message;
            }

            if (deserializedData.IsInitialized())
            {
                message = createDefaultErrorMessage();
                std::string protobufErrorListString = deserializedData.InitializationErrorString();
                std::stringstream errorMessage;
                errorMessage << "Bad formed message : " << protobufErrorListString;
                message.Error = errorMessage.str();

                LOGERROR("Protocol Serializer error: " << message.Error);
                return message;
            }

            message.ApplicationId = deserializedData.applicationid();
            message.PluginName = deserializedData.pluginname();
            message.Command = PluginProtocol::DeserializeCommand(deserializedData.command());
            message.MessageId = deserializedData.messageid();

            if(message.Command == PluginProtocol::Commands::UNKNOWN)
            {
                message.Error = "Invalid request";
                LOGERROR("Protocol Serializer error: " << message.Error);
            }

            if (deserializedData.has_error()) {
                message.Error = deserializedData.error();
                LOGERROR("Protocol Serializer message error: " << deserializedData.error());
                return message;
            }

            if (deserializedData.payload_size() > 0)
            {
                for (auto & payload_content : deserializedData.payload())
                {
                    message.Payload.push_back(payload_content);
                }
            }

            return message;
        }

        Common::PluginProtocol::DataMessage ProtocolSerializer::createDefaultErrorMessage()
        {
            Common::PluginProtocol::DataMessage dataMessage;
            dataMessage.Command = Common::PluginProtocol::Commands::UNKNOWN;

            return dataMessage;
        }


    }

}

