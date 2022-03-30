/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once
#include "MCSHttpClient.h"
#include <string>
#include <map>

namespace MCS
{
class MCSApiCalls
{
    public:
        std::string getJwToken(MCSHttpClient client);
        void registerEndpoint(MCSHttpClient& client, std::map<std::string, std::string>& registerConfig);
};

}