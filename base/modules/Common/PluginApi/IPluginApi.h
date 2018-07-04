/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINAPI_H
#define EVEREST_BASE_PLUGINAPI_H

#include "IPluginCallback.h"
#include "ISensorDataCallback.h"
#include <memory>

namespace Common
{
    namespace PluginApi
    {
        class IPluginApi
        {
        public:
            virtual ~IPluginApi() = default;

            virtual void sendEvent(const std::string& appId, const std::string& eventXml) const  = 0;

            /**
            * @param statusWithoutTimestampsXml - status Xml without timestamps so we can see if the status has changed in an
              important way, ignoring time.
            */
            virtual void changeStatus(const std::string& appId, const std::string& statusXml, const std::string& statusWithoutTimestampsXml) const = 0;

            virtual std::string getPolicy(const std::string &appId) const = 0;

        };

        //std::string getLibraryVersion();
    }
}


#endif //EVEREST_BASE_PLUGINAPI_H
