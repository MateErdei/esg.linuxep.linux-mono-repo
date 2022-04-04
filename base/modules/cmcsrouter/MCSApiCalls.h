/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include "MCSHttpClient.h"
#include "ConfigOptions.h"

#include <string>
#include <map>

namespace MCS
{
class MCSApiCalls
{
    public:
        std::string getJwToken(MCSHttpClient client);
        bool registerEndpoint(
            MCSHttpClient& client,
            MCS::ConfigOptions& configOptions,
            const std::string& statusXml,
            std::string proxy);
        std::string preregisterEndpoint(
            MCSHttpClient& client,
            MCS::ConfigOptions& registerConfig,
            const std::string& statusXml,
            std::string proxy);

};

}