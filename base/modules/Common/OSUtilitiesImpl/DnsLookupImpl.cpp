/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DnsLookupImpl.h"

#include <Common/OSUtilities/IIPUtils.h>
#include <net/if.h>

#include <cstring>
#include <ifaddrs.h>
#include <netdb.h>
#include <sstream>

namespace
{
    using Common::OSUtilities::IPs;
    using Common::OSUtilitiesImpl::IDnsLookupPtr;
    IDnsLookupPtr& LocalDnsStaticPointer()
    {
        static IDnsLookupPtr instance = IDnsLookupPtr(new Common::OSUtilitiesImpl::DnsLookupImpl());
        return instance;
    }

    struct AddrInfoScopeGuard
    {
        struct addrinfo* addr;
        explicit AddrInfoScopeGuard(struct addrinfo* addr_) : addr(addr_) {}
        ~AddrInfoScopeGuard() { freeaddrinfo(addr); }
    };

    IPs ipResolution(const std::string& uri)
    {
        struct addrinfo hints; // NOLINT
        struct addrinfo* ifa;
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_DGRAM; /* Any socket type */

        int err = getaddrinfo(uri.c_str(), nullptr, &hints, &ifa);
        if (err != 0)
        {
            std::stringstream ss;
            ss << "Ip resolution failed: " << uri << " error: " << gai_strerror(err);
            throw std::runtime_error(ss.str());
        }
        AddrInfoScopeGuard scopeGuard{ ifa };
        IPs ips;
        for (; ifa != nullptr; ifa = ifa->ai_next)
        {
            if (ifa->ai_addr == nullptr)
                continue;

            int family = ifa->ai_addr->sa_family;

            if (family == AF_INET)
            {
                struct sockaddr_in* ipSockAddr = reinterpret_cast<struct sockaddr_in*>(ifa->ai_addr); // NOLINT
                ips.ip4collection.emplace_back(ipSockAddr);
            }
            else if (family == AF_INET6)
            {
                struct sockaddr_in6* ipSockAddr = reinterpret_cast<struct sockaddr_in6*>(ifa->ai_addr); // NOLINT
                ips.ip6collection.emplace_back(ipSockAddr);
            }
        }

        return ips;
    }

} // namespace

namespace Common
{
    namespace OSUtilitiesImpl
    {
        Common::OSUtilities::IPs DnsLookupImpl::lookup(const std::string& uri) const { return ::ipResolution(uri); }

        void replaceDnsLookup(IDnsLookupPtr other) { LocalDnsStaticPointer().reset(other.release()); }

        void restoreDnsLookup() { LocalDnsStaticPointer().reset(new DnsLookupImpl()); }

    } // namespace OSUtilitiesImpl
} // namespace Common

Common::OSUtilities::IDnsLookup* Common::OSUtilities::dnsLookup()
{
    return LocalDnsStaticPointer().get();
}
