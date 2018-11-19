/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "IIPUtils.h"
#include <Common/OSUtilities/IDnsLookup.h>
#include <Common/OSUtilities/ILocalIP.h>

#include <sstream>
#include <future>
#include <algorithm>

#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
namespace
{
    using Common::OSUtilities::IP4;
    using Common::OSUtilities::IP6;
    using Common::OSUtilities::IPs;
    using Common::OSUtilities::Ip6addr;
    using Common::OSUtilities::Ip4addr;


    template<typename CollectionType>
    int minDistanceCollection(const CollectionType& localCollection, const CollectionType& remoteCollection)
    {
        int minCurrentValue = 128;
        for (const auto& local: localCollection)
        {
            for (const auto& remote: remoteCollection)
            {
                minCurrentValue = std::min(minCurrentValue, local.distance(remote));
            }
        }
        return minCurrentValue;
    }

    int minDistance(const IPs& localIps, const IPs& remoteIps)
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

    std::vector<ServerURI> dnsLookup(const std::vector<std::string>& serversuri)
    {
        auto dnsLookupPtr = Common::OSUtilities::dnsLookup();
        std::vector<ServerURI> servers;
        std::vector<std::future<ServerURI>> tasks;

        for (const auto& uri : serversuri)
        {
            tasks.emplace_back(std::async([uri, &dnsLookupPtr]() -> ServerURI
            {
                  ServerURI serverURI;
                  serverURI.uri = uri;
                  try
                  {
                      auto ips = dnsLookupPtr->lookup(uri);
                      serverURI.ips = ips;
                  }
                  catch (std::exception& ex)
                  {
                      serverURI.error = ex.what();
                  }
                  return serverURI;

            }
            ));
        }
        for (auto& fut : tasks)
        {
            servers.emplace_back(fut.get());
        }

        return servers;
    }
}
namespace
{
    template <typename SockAddr>
    std::string ip2string( SockAddr * sockAddr )
    {
        static_assert( std::is_same<SockAddr,  struct sockaddr_in>::value ||
                                    std::is_same<SockAddr,  struct sockaddr_in6>::value
                , "Must be either sockaddr_in or sockaddr_in6");
        char host[NI_MAXHOST];
        const struct sockaddr * sockaddr1 = reinterpret_cast<const struct sockaddr*>(sockAddr);
        int s = getnameinfo(sockaddr1, sizeof( SockAddr), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
        if ( s != 0 )
        {
            std::stringstream ss;
            ss << "convert sock address failed: " << gai_strerror(s);
            throw std::runtime_error( ss.str());
        }
        return std::string( host);
    }

    Common::OSUtilities::Ip6addr convert(uint8_t (&ip6array)[16] )
    {
        Common::OSUtilities::Ip6addr  ip6addr;
        std::copy(ip6array, ip6array+16, ip6addr.begin() );
        return ip6addr;
    }


}
namespace Common
{
    namespace OSUtilities
    {
        // IP4

        IP4::IP4(struct sockaddr_in* ipSockAddr)
        {
            m_ip4addr = ipSockAddr->sin_addr.s_addr;
            m_address = ip2string(ipSockAddr);
        }

        int IP4::distance(const Common::OSUtilities::IP4& other) const
        {

            Ip4addr differentBits =  m_ip4addr ^ other.m_ip4addr;
            return bitLength(differentBits);
        }

        int IP4::bitLength(const Common::OSUtilities::Ip4addr& ip4addr) const
        {
            uint32_t hostEquivalent = ntohl(ip4addr);
            for( int i =31; i>0; i--)
            {
                uint32_t mask = 1 << i;
                if ( hostEquivalent & mask)
                {
                    return i+1;
                }
            }
            return 0;
        }

        IP4::IP4(const std::string& stringAddress)
        {
            in_addr_t ip = inet_addr( stringAddress.c_str());
            if( ip == -1 )
            {
                std::stringstream s ;
                s<< "Invalid ip address: " << stringAddress;
                throw std::runtime_error(s.str());
            }
            m_address = stringAddress;
            m_ip4addr = ip;
        }


        // IP6
        IP6::IP6(struct sockaddr_in6* ipSockAddr)
        {
            m_ip6addr = ::convert(ipSockAddr->sin6_addr.s6_addr);
            m_address = ip2string(ipSockAddr);
        }

        int IP6::distance(const Common::OSUtilities::IP6& other) const
        {
            Ip6addr differentBits;
            for(int i =0; i<16; i++)
            {
                differentBits[i] = m_ip6addr[i] ^ other.m_ip6addr[i];
            }

            auto result = bitLength(differentBits);
            return result;
        }

        int IP6::bitLength(const Common::OSUtilities::Ip6addr& ip6addr) const
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