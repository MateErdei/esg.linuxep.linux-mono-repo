/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>
#include <vector>

namespace Common
{
    namespace PluginProtocol
    {

        enum class Commands{UNKNOWN,
                            PLUGIN_SEND_EVENT,
                            PLUGIN_SEND_STATUS,
                            PLUGIN_SEND_REGISTER,
                            PLUGIN_QUERY_CURRENT_POLICY,
                            REQUEST_PLUGIN_APPLY_POLICY,
                            REQUEST_PLUGIN_DO_ACTION,
                            REQUEST_PLUGIN_STATUS,
                            REQUEST_PLUGIN_TELEMETRY};

        struct DataMessage
        {
            /**
                      * ProtocalVersion, used to identify what implementation of the message serialise and deserialise should be used.
                      * to process the message.
                      */
            std::string ProtocolVersion;

            // see https://wiki.sophos.net/display/SophosCloud/Endpoint+Management+Protocol
            /**
             * ApplicationId, used to store identifier for the application i.e. ALC, SAV etc..
             */
            std::string ApplicationId;

            /**
             * PluginName, unique name of the plugin the data message is for.
             */
            std::string PluginName;

            /**
             * COMMAND, The type that the message should map to.
             */
            Commands Command;

            /**
             * MessageId, used to tag the message with a unique id.
             */
            std::string MessageId;

            /**
             * Error, used to store error message if required.
             */
            std::string Error;

            /**
             * Payload, vector used to store the actual message data.
             */
            std::vector<std::string> Payload;
        };

        std::string SerializeCommand( Commands  command);
        Commands DeserializeCommand( const std::string & command_str);

    }
}




