/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_DATAMESSAGE_H
#define EVEREST_BASE_DATAMESSAGE_H

#include <string>
#include <vector>

namespace Common
{
    namespace PluginApi
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

        struct RegistrationInfo
        {
            std::string m_pluginName;
            std::vector<std::string> m_appIds;
        };

        struct DataMessage
        {
            std::string ProtocolVersion;
            std::string ApplicationId;
            Commands Command;
            std::string MessageId;
            std::string Error;
            std::vector<std::string> payload;
        };

        std::string SerializeCommand( Commands  command);
        Commands DeserializeCommand( const std::string & command_str);

    }
}



#endif //EVEREST_BASE_DATAMESSAGE_H
