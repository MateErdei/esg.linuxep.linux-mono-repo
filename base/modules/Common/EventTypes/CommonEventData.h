/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common
{
    namespace EventTypes
    {
        struct UserSid
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

        struct SocketAddress
        {
        public:
            std::string address;
            unsigned int port;
        };

        struct IpFlow
        {
        public:
            SocketAddress sourceAddress;
            SocketAddress destinationAddress;
            unsigned short protocol;

        };
    }
}