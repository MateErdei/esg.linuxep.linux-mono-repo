//
// Created by pair on 29/06/18.
//
#include "DataMessage.h"
#include <unordered_map>
#include <thread>
#include <mutex>
namespace
{
    using CommandMap = std::unordered_map<std::string, Common::PluginApi::Commands >;
    void setupMapString ( CommandMap & commandMap)
    {
        using namespace Common::PluginApi;
        for ( auto command: {Commands::PLUGIN_SEND_EVENT,
                             Commands::PLUGIN_SEND_STATUS,
                             Commands::PLUGIN_SEND_REGISTER,
                             Commands::PLUGIN_QUERY_CURRENT_POLICY,
                             Commands::REQUEST_PLUGIN_APPLY_POLICY,
                             Commands::REQUEST_PLUGIN_DO_ACTION,
                             Commands::REQUEST_PLUGIN_STATUS,
                             Commands::REQUEST_PLUGIN_TELEMETRY})
        {
            commandMap[SerializeCommand(command)] = command;
        }
    }
    const CommandMap& mapStringCommand()
    {
        static std::mutex mutex;
        static  CommandMap mapStringCommand;

        if ( mapStringCommand.empty())
        {
            std::lock_guard<std::mutex> lock(mutex);
            if( mapStringCommand.empty())
            {
                setupMapString(mapStringCommand);
            }
        }
        return mapStringCommand;
    }

}

namespace Common
{
    namespace PluginApi
    {
        std::string SerializeCommand( Commands  command)
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

        Commands DeserializeCommand( const std::string & command_str)
        {
            auto found = mapStringCommand().find(command_str);
            if (found != mapStringCommand().end() )
            {
                return found->second;
            }
            return Commands::UNKNOWN;
        }
    }
}
