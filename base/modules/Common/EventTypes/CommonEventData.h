/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common
{
    namespace EventTypes
    {
        struct UserSID
        {
        public:
            std::string username;
            std::string sid;
            std::string domain;

        };

        struct NetworkAddress
        {
        public:
            std::string address;
        };
    }
}