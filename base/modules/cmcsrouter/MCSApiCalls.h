/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include "MCSHttpClient.h"
#include "ConfigOptions.h"

#include <json.hpp>

#include <string>
#include <map>

namespace MCS
{
class MCSApiCalls
{
    public:
        std::map<std::string,std::string> getAuthenticationInfo(MCSHttpClient client);
        bool registerEndpoint(
            MCSHttpClient& client,
            MCS::ConfigOptions& configOptions,
            const std::string& statusXml,
            const std::string& proxy
        );
        std::string preregisterEndpoint(
            MCSHttpClient& client,
            MCS::ConfigOptions& registerConfig,
            const std::string& statusXml,
            const std::string& proxy
        );
};

}