/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DataMessage.h"
#include <unordered_map>
#include <thread>
#include <mutex>

namespace Common
{
    namespace PluginProtocol
    {
        std::string SerializeCommand(Commands command)
        {
            switch (command)
            {
                case Commands::PLUGIN_SEND_EVENT:
                    return "SendEvent";
                case Commands::PLUGIN_SEND_STATUS:
                    return "SendStatus";
                case Commands::PLUGIN_SEND_REGISTER:
                    return "Registration";
                case Commands::REQUEST_PLUGIN_APPLY_POLICY:
                    return "ApplyPolicy";
                case Commands::REQUEST_PLUGIN_DO_ACTION:
                    return "DoAction";
                case Commands::REQUEST_PLUGIN_STATUS:
                    return "RequestStatus";
                case Commands::REQUEST_PLUGIN_TELEMETRY:
                    return "Telemetry";
                case Commands::PLUGIN_QUERY_CURRENT_POLICY:
                    return "RequestCurrentPolicy";
                case Commands::UNKNOWN:
                default:
                    return "InvalidCommand";

            }
        }

        Commands DeserializeCommand(const std::string& command_str)
        {
            if (command_str == SerializeCommand(Commands::PLUGIN_SEND_EVENT))
            {
                return Commands::PLUGIN_SEND_EVENT;
            }
            else if (command_str == SerializeCommand(Commands::PLUGIN_SEND_STATUS))
            {
                return Commands::PLUGIN_SEND_STATUS;
            }
            else if (command_str == SerializeCommand(Commands::PLUGIN_SEND_REGISTER))
            {
                return Commands::PLUGIN_SEND_REGISTER;
            }
            else if (command_str == SerializeCommand(Commands::REQUEST_PLUGIN_APPLY_POLICY))
            {
                return Commands::REQUEST_PLUGIN_APPLY_POLICY;
            }
            else if (command_str == SerializeCommand(Commands::REQUEST_PLUGIN_DO_ACTION))
            {
                return Commands::REQUEST_PLUGIN_DO_ACTION;
            }
            else if (command_str == SerializeCommand(Commands::REQUEST_PLUGIN_STATUS))
            {
                return Commands::REQUEST_PLUGIN_STATUS;
            }
            else if (command_str == SerializeCommand(Commands::REQUEST_PLUGIN_TELEMETRY))
            {
                return Commands::REQUEST_PLUGIN_TELEMETRY;
            }
            else if (command_str == SerializeCommand(Commands::PLUGIN_QUERY_CURRENT_POLICY))
            {
                return Commands::PLUGIN_QUERY_CURRENT_POLICY;
            }
            return Commands::UNKNOWN;
        }
    }
}
