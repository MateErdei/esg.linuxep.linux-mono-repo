/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
namespace Common
{
    namespace OSUtilities
    {
        using Ip6addr = std::array<uint8_t, 16>;
        using Ip4addr = uint32_t;

        struct IP4
        {
            std::string address;
            Ip4addr   ip4addr;
        };

        struct IP6
        {
            std::string address;
            Ip6addr   ip6addr;
        };

        struct IPs
        {
            std::vector<IP4> ip4collection;
            std::vector<IP6> ip6collection;
        };

        std::vector<int> indexOfSortedURIsByIPProximity(const std::vector<std::string> & servers);

    }
}