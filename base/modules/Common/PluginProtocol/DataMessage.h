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
        enum class Commands
        {
            UNSET,
            UNKNOWN,
            PLUGIN_SEND_EVENT,
            PLUGIN_SEND_STATUS,
            PLUGIN_SEND_REGISTER,
            PLUGIN_QUERY_CURRENT_POLICY,
            REQUEST_PLUGIN_APPLY_POLICY,
            REQUEST_PLUGIN_DO_ACTION,
            REQUEST_PLUGIN_STATUS,
            REQUEST_PLUGIN_TELEMETRY
        };

        struct DataMessage
        {
            DataMessage() : m_command(Commands::UNSET), m_acknowledge(false) {}
            DataMessage(
                std::string applicationid,
                std::string pluginName,
                Commands command,
                std::string error,
                bool acknowledge,
                std::vector<std::string> payload) :
                m_applicationId(std::move(applicationid)),
                m_pluginName(std::move(pluginName)),
                m_command(command),
                m_error(std::move(error)),
                m_acknowledge(acknowledge),
                m_payload(std::move(payload))
            {
            }
            // see https://wiki.sophos.net/display/SophosCloud/Endpoint+Management+Protocol
            /**
             * m_applicationId, used to store identifier for the application i.e. ALC, SAV etc..
             */
            std::string m_applicationId;

            /**
             * m_pluginName, unique name of the plugin the data message is for.
             */
            std::string m_pluginName;

            /**
             * m_command, The type that the message should map to.
             */
            Commands m_command;

            /**
             * m_error, used to store error message if required.
             */
            std::string m_error;

            /**
             * m_acknowledge, boolean that denotes the acknowledgement of a message received.
             */
            bool m_acknowledge;

            /**
             * m_payload, vector used to store the actual message data.
             */
            std::vector<std::string> m_payload;

            /**
             * correlation id refers to the id of the command received by central.
             * Currently, it is populated only by Command Request (LiveQuery)
             */
            std::string m_correlationId;
        };

        std::string ConvertCommandEnumToString(Commands command);

    } // namespace PluginProtocol
} // namespace Common
