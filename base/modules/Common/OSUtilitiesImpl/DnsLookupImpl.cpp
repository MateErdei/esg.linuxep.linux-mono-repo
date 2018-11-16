/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DnsLookupImpl.h"
#include "IPUtilsImpl.h"
#include <sstream>

#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <net/if.h>


namespace
{
    using Common::OSUtilitiesImpl::IDnsLookupPtr;
    using Common::OSUtilities::IPs;
    using Common::OSUtilitiesImpl::fromIP4;
    using Common::OSUtilitiesImpl::fromIP6;
    IDnsLookupPtr& LocalDnsStaticPointer()
    {
        static IDnsLookupPtr instance = IDnsLookupPtr(new Common::OSUtilitiesImpl::DnsLookupImpl());
        return instance;
    }


    struct AddrInfoScopeGuard
    {
        struct addrinfo *addr;
        explicit AddrInfoScopeGuard(struct addrinfo *addr_):addr(addr_){}
        ~AddrInfoScopeGuard()
        {
            freeaddrinfo(addr);
        }
    };


    IPs ipResolution( const std::string & uri)
    {
        struct addrinfo hints; // NOLINT
        struct addrinfo *ifa;
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_DGRAM; /* Any socket type */

        int err = getaddrinfo(uri.c_str(), nullptr, &hints, &ifa);
        if (err != 0)
        {
            std::stringstream ss;
            ss << "Ip resolution failed: " << uri << " error: " << gai_strerror(err);
            throw std::runtime_error( ss.str());
        }
        AddrInfoScopeGuard scopeGuard{ifa};
        IPs ips;
        for (; ifa != nullptr; ifa = ifa->ai_next)
        {
            if (ifa->ai_addr == nullptr)
                continue;

            int family = ifa->ai_addr->sa_family;

            if( family == AF_INET)
            {
                ips.ip4collection.emplace_back( fromIP4(ifa->ai_addr) );
            }
            else if ( family == AF_INET6)
            {
                ips.ip6collection.emplace_back( fromIP6(ifa->ai_addr));
            }
        }

        return ips;
    }

}


namespace Common
{
    namespace OSUtilitiesImpl
    {
        Common::OSUtilities::IPs DnsLookupImpl::lookup(const std::string& uri) const
        {
            return ::ipResolution(uri);
        }

        void replaceDnsLookup(IDnsLookupPtr other)
        {
            LocalDnsStaticPointer().reset(other.release());
        }

        void restoreDnsLookup()
        {
            LocalDnsStaticPointer().reset(new DnsLookupImpl());
        }

    }
}

Common::OSUtilities::IDnsLookup* Common::OSUtilities::dnsLookup()
{
    return LocalDnsStaticPointer().get();
}
