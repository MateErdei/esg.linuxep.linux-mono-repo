//
// Created by pair on 06/07/18.
//

#ifndef EVEREST_BASE_STATUSINFO_H
#define EVEREST_BASE_STATUSINFO_H

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
        /// @see IPluginApi::changeStatus
        std::string statusWithoutXml;
        std::string appId;
    };
}
}


#endif //EVEREST_BASE_STATUSINFO_H
