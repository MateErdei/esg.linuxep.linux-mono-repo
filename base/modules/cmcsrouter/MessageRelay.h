/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace MCS
{
    struct MessageRelay
    {
        std::string priority;
        std::string id;
        std::string address;
        std::string port;
    };
}