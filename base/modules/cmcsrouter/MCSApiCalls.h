/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once
#include "MCSHttpClient.h"
#include <string>
namespace MCS
{
class MCSApiCalls
{
    public:
        std::string getJwToken(MCSHttpClient client);
};

}