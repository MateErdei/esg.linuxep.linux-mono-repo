/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IPUtilsImpl.h"
#include <Common/OSUtilities/IDnsLookup.h>
#include <Common/OSUtilities/ILocalIP.h>
#include <algorithm>
#include <sstream>
#include <future>
#include <netdb.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <net/if.h>

namespace
{
    using Common::OSUtilities::IP4;
    using Common::OSUtilities::IP6;
    using Common::OSUtilities::IPs;
    using Common::OSUtilities::Ip6addr;
    using Common::OSUtilities::Ip4addr;

    Ip6addr convert(uint8_t (&ip6array)[16] )
    {
        Ip6addr  ip6addr;
        std::copy(ip6array, ip6array+16, ip6addr.begin() );
        return ip6addr;
    }

    int bitLength( Ip4addr ip4addr)
    {
        for( int i =31; i>0; i--)
        {
            Ip4addr mask = 1 << i;
            if ( ip4addr & mask)
            {
                return i+1;
            }
        }
        return 0;
    }

    int bitLength( Ip6addr ip6addr)
    {
        for( int i =0; i<16; i++)
        {
            const uint8_t  & byte = ip6addr[i];
            for( int j=7; j>=0; j--)
            {
                uint8_t mask = 1 << j;
                if( byte & mask)
                {
                    return (16-i)*8 -(7-j);
                }
            }
        }
        return 0;
    }


    int ipDistance(const IP4 & lh, const IP4 & rh)
    {
        Ip4addr differentBits = lh.ip4addr ^ rh.ip4addr;

        auto result = bitLength(differentBits);
        return result;
    }

    int ipDistance(const IP6 & lh, const IP6 & rh)
    {
        Ip6addr differentBits;
        for(int i =0; i<16; i++)
        {
            differentBits[i] = lh.ip6addr[i] ^ rh.ip6addr[i];
        }

        auto result = bitLength(differentBits);
        return result;
    }

    template <typename CollectionType>
    int minDistanceCollection(const CollectionType & localCollection, const CollectionType & remoteCollection )
    {
        int minCurrentValue= 128;
        for( const auto & local: localCollection)
        {
            for( const auto & remote: remoteCollection)
            {
                minCurrentValue = std::min( minCurrentValue, ipDistance(local, remote));
            }
        }
        return minCurrentValue;
    }

    int minDistance( const IPs & localIps, const IPs & remoteIps)
    {
        return std::min(
                minDistanceCollection(localIps.ip4collection, remoteIps.ip4collection),
                minDistanceCollection(localIps.ip6collection, remoteIps.ip6collection)
        );
    }

    struct ServerURI
    {
        std::string uri;
        IPs ips;
        std::string error = "";
        int associatedMinDistance = 129;
        int originalIndex = 0;
    };

    std::vector<ServerURI> dnsLookup(const std::vector<std::string> & serversuri)
    {
        auto dnsLookupPtr = Common::OSUtilities::dnsLookup();
        std::vector<ServerURI> servers;
        std::vector<std::future<ServerURI>> tasks;

        for( const auto & uri : serversuri)
        {
            tasks.emplace_back( std::async([uri, &dnsLookupPtr]()->ServerURI{
                ServerURI serverURI;
                serverURI.uri = uri;
                try
                {
                    auto ips = dnsLookupPtr->lookup(uri);
                    serverURI.ips = ips;
                }
                catch(std::exception & ex)
                {
                    serverURI.error = ex.what();
                }
                return  serverURI;
            }));
        }
        for( auto & fut : tasks)
        {
            servers.emplace_back(fut.get());
        }

        return servers;
    }


}

namespace Common
{
    namespace OSUtilitiesImpl
    {

        Common::OSUtilities::IP4 fromIP4(struct sockaddr * ifa_addr)
        {
            char host[NI_MAXHOST];
            int s = getnameinfo(ifa_addr, sizeof( struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if ( s != 0 )
            {
                std::stringstream ss;
                ss << "convert sock address failed: " << gai_strerror(s);
                throw std::runtime_error( ss.str());
            }
            struct sockaddr_in *ipSockAddr = reinterpret_cast<struct  sockaddr_in*>(ifa_addr); // NOLINT
            return Common::OSUtilities::IP4{host, ntohl(ipSockAddr->sin_addr.s_addr)};
        }

        Common::OSUtilities::IP6 fromIP6(struct sockaddr * ifa_addr)
        {
            char host[NI_MAXHOST];
            int err = getnameinfo(ifa_addr, sizeof( struct sockaddr_in6), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if ( err != 0 )
            {
                std::stringstream ss;
                ss << "convert sock address failed: " << gai_strerror(err);
                throw std::runtime_error( ss.str());
            }
            struct sockaddr_in6 *ipSockAddr = reinterpret_cast<struct  sockaddr_in6*>(ifa_addr); // NOLINT
            return Common::OSUtilities::IP6{host, convert(ipSockAddr->sin6_addr.s6_addr)};
        }


    }
}

namespace Common
{
    namespace OSUtilities
    {
        std::vector<int> indexOfSortedURIsByIPProximity(const std::vector<std::string> & servers)
        {

            auto localips = Common::OSUtilities::localIP()->getLocalIPs();
            //std::cout << "Local ips: " << localips << std::endl;
            auto lookups = ::dnsLookup(servers);
            int i=0;
            for( auto & entry : lookups)
            {
                entry.originalIndex = i++;
                if( entry.error.empty())
                {
                    entry.associatedMinDistance = minDistance(localips, entry.ips);
                }
            }

            std::stable_sort(lookups.begin(), lookups.end(),
                             [](const ServerURI & lh, const ServerURI & rh){return lh.associatedMinDistance < rh.associatedMinDistance; });

            std::vector<int> sortedIndexes;
            for( const auto & entry : lookups)
            {
                sortedIndexes.emplace_back(entry.originalIndex);
            }

            return sortedIndexes;
        }
    }
}


