/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace MCS
{
    struct MessageRelay
    {
        int priority;
        std::string id;
        std::string address;
        std::string port;

        bool operator==(const MessageRelay& other) const
        {
            return priority == other.priority && id == other.id && address == other.address && port == other.port;
        }
    };
} // namespace MCS