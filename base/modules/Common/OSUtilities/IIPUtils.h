/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <array>
#include <cstdint>
#include <netdb.h>
#include <string>
#include <vector>

namespace Common
{
    namespace OSUtilities
    {
        using Ip6addr = std::array<uint8_t, 16>;
        using Ip4addr = uint32_t;

        /// Auxiliary struct to handle IP4 entries.
        class IP4
        {
            std::string m_address;
            Ip4addr m_ip4addr;
            int bitLength(const Ip4addr& ip4addr) const;

        public:
            explicit IP4(struct sockaddr_in*);
            explicit IP4(const std::string& stringAddress);
            int distance(const IP4& other) const;
            std::string stringAddress() const { return m_address; };
            Ip4addr ipAddress() const { return m_ip4addr; };
        };

        /// Auxiliary struct to handle IP6 entries.
        struct IP6
        {
            std::string m_address;
            Ip6addr m_ip6addr;
            int bitLength(const Ip6addr& ip6addr) const;

        public:
            explicit IP6(struct sockaddr_in6*);
            int distance(const IP6& other) const;
            std::string stringAddress() const { return m_address; };
            // Ip6addr ipAddress() const{ return m_ip6addr; };
        };

        struct IPs
        {
            std::vector<IP4> ip4collection;
            std::vector<IP6> ip6collection;
        };

        /**
         * Extract the server address from http, https paths.
         * For example:
         * http://server.com:898/path/to/something return server.com
         *
         * If it can not extract the server path it returns the input as the 'best guess' for the server address.
         * @return
         */
        std::string tryExtractServerFromHttpURL(const std::string&);

        /**
         * Struct to hold information about remote server with its associated ips and the distance associated to that
         * server.
         */
        struct ServerURI
        {
            constexpr static int MaxDistance = 129;
            std::string uri;
            IPs ips;
            std::string error = "";
            int associatedMinDistance = MaxDistance;
            int originalIndex = -1;
        };

        struct SortServersReport
        {
            IPs localIps;
            std::vector<ServerURI> servers;
        };
        /**
         * Sort the servers by ip distance, where ip distance is defined as the minimun bit length of the xor of local
         * ip and remote ip.
         *
         * The answer return the servers in the correct order of distance, but also give the originalIndex. This can be
         * used to sort structs that originated the servers input.
         *
         * It also provide the full information of the Report to allow for logging how the distance was calculated.
         *
         * If it can not get a dns resolution of a server, that server is given distance associated as
         * ServerURI::MaxDistance and the sort algorithm will keep its relative order (stable sort)
         *
         * @param servers
         * @return SortServersReport
         */
        SortServersReport indexOfSortedURIsByIPProximity(const std::vector<std::string>& servers);
        std::vector<int> sortedIndexes(const SortServersReport& report);
    } // namespace OSUtilities
} // namespace Common