/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LocalIPImpl.h"

#include <Common/OSUtilities/IIPUtils.h>
#include <net/if.h>

#include <cstring>
#include <ifaddrs.h>
#include <netdb.h>
#include <sstream>

namespace
{
    using Common::OSUtilities::IPs;
    using Common::OSUtilitiesImpl::ILocalIPPtr;

    ILocalIPPtr& LocalIPStaticPointer()
    {
        static ILocalIPPtr instance = ILocalIPPtr(new Common::OSUtilitiesImpl::LocalIPImpl());
        return instance;
    }
    struct IfAddrScopeGuard
    {
        struct ifaddrs* addr;
        explicit IfAddrScopeGuard(struct ifaddrs* ifAddr_) : addr(ifAddr_) {}
        ~IfAddrScopeGuard() { freeifaddrs(addr); }
    };
    IPs localIPs()
    {
        struct ifaddrs* listIFAddr;

        if (getifaddrs(&listIFAddr) == -1)
        {
            throw std::system_error(errno, std::generic_category(), "Failed to get local ip information.");
        }

        // ensure the entire list is cleared on return
        IfAddrScopeGuard ifAddrScopeGuard{ listIFAddr };
        IPs ips;
        // walk through the list
        for (; listIFAddr != nullptr; listIFAddr = listIFAddr->ifa_next)
        {
            if (listIFAddr->ifa_addr == nullptr)
                continue;

            if (listIFAddr->ifa_flags & IFF_LOOPBACK)
                continue;

            int family = listIFAddr->ifa_addr->sa_family;

            if (family == AF_INET)
            {
                struct sockaddr_in* ipSockAddr = reinterpret_cast<struct sockaddr_in*>(listIFAddr->ifa_addr); // NOLINT
                ips.ip4collection.emplace_back(ipSockAddr);
            }
            else if (family == AF_INET6)
            {
                struct sockaddr_in6* ipSockAddr =
                    reinterpret_cast<struct sockaddr_in6*>(listIFAddr->ifa_addr); // NOLINT
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
        Common::OSUtilities::IPs LocalIPImpl::getLocalIPs() const { return ::localIPs(); }

        void replaceLocalIP(ILocalIPPtr other) { LocalIPStaticPointer().reset(other.release()); }

        void restoreLocalIP() { LocalIPStaticPointer().reset(new Common::OSUtilitiesImpl::LocalIPImpl()); }
    } // namespace OSUtilitiesImpl
} // namespace Common

Common::OSUtilities::ILocalIP* Common::OSUtilities::localIP()
{
    return LocalIPStaticPointer().get();
}
