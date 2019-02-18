/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common
{
    namespace PluginApi
    {
        /**
         * Struct to enable replying to status query from Management Agent.
         * @see IPluginApi::changeStatus
         */
        struct StatusInfo
        {
            /// Content of the status that can be sent to Sophos Cloud
            std::string statusXml;
            /// Representation of the status that can be used to reliably identify when it has changed.
            /// @see IBaseServiceApi::changeStatus
            std::string statusWithoutTimestampsXml;
            /// Application Id that the status is related to
            std::string appId;
        };
    } // namespace PluginApi
} // namespace Common
