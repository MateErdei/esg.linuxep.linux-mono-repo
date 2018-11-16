/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include <netdb.h>

namespace Common
{
    namespace OSUtilities
    {
        using Ip6addr = std::array<uint8_t, 16>;
        using Ip4addr = uint32_t;

        class IP4
        {
            std::string m_address;
            Ip4addr   m_ip4addr;
            int bitLength(const Ip4addr & ip4addr ) const;
        public:
            explicit IP4( struct sockaddr_in * );
            int distance( const IP4 & other) const;
            std::string stringAddress() const {return m_address;};
            //Ip4addr ipAddress() const{ return m_ip4addr; };
        };

        struct IP6
        {
            std::string m_address;
            Ip6addr   m_ip6addr;
            int bitLength(const Ip6addr & ip6addr  ) const;
        public:
            explicit IP6( struct sockaddr_in6 * );
            int distance( const IP6 & other) const;
            std::string stringAddress() const {return m_address;};
            //Ip6addr ipAddress() const{ return m_ip6addr; };
        };

        struct IPs
        {
            std::vector<IP4> ip4collection;
            std::vector<IP6> ip6collection;
        };



        std::vector<int> indexOfSortedURIsByIPProximity(const std::vector<std::string> & servers);

    }
}