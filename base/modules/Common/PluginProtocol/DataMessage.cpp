/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DataMessage.h"

#include <mutex>
#include <thread>
#include <unordered_map>

namespace Common
{
    namespace PluginProtocol
    {
        std::string ConvertCommandEnumToString(Commands command)
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
    } // namespace PluginProtocol
} // namespace Common
