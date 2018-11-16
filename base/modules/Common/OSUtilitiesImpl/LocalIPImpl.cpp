/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LocalIPImpl.h"
#include "IPUtilsImpl.h"
#include <sstream>

#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <net/if.h>

namespace
{
    using Common::OSUtilitiesImpl::ILocalIPPtr;
    using Common::OSUtilities::IPs;
    using Common::OSUtilitiesImpl::fromIP4;
    using Common::OSUtilitiesImpl::fromIP6;

    ILocalIPPtr& LocalIPStaticPointer()
    {
        static ILocalIPPtr instance = ILocalIPPtr(new Common::OSUtilitiesImpl::LocalIPImpl());
        return instance;
    }
    struct IfAddrScopeGuard
    {
        struct ifaddrs *addr;
        explicit IfAddrScopeGuard(struct ifaddrs *ifAddr_ ) : addr(ifAddr_)
        {
        }
        ~IfAddrScopeGuard()
        {
            freeifaddrs(addr);
        }
    };
    IPs localIPs()
    {

        struct ifaddrs *listIFAddr;

        if (getifaddrs(&listIFAddr) == -1)
        {
            throw std::system_error(errno, std::generic_category(), "Failed to get local ip information.");
        }

        // ensure the entire list is cleared on return
        IfAddrScopeGuard ifAddrScopeGuard{listIFAddr};
        IPs ips;
        // walk through the list
        for (; listIFAddr != nullptr; listIFAddr = listIFAddr->ifa_next) {
            if (listIFAddr->ifa_addr == nullptr)
                continue;

            if (listIFAddr->ifa_flags & IFF_LOOPBACK)
                continue;

            int family = listIFAddr->ifa_addr->sa_family;

            if( family == AF_INET)
            {
                ips.ip4collection.emplace_back( fromIP4(listIFAddr->ifa_addr) );
            }
            else if ( family == AF_INET6)
            {
                ips.ip6collection.emplace_back( fromIP6(listIFAddr->ifa_addr));
            }
        }
        return ips;
    }

}

namespace Common
{
    namespace OSUtilitiesImpl
    {

        Common::OSUtilities::IPs LocalIPImpl::getLocalIPs() const
        {
            return ::localIPs();
        }

        void replaceLocalIP(ILocalIPPtr other)
        {
            LocalIPStaticPointer().reset(other.release());

        }

        void restoreLocalIP()
        {
            LocalIPStaticPointer().reset( new Common::OSUtilitiesImpl::LocalIPImpl());
        }
    }
}

Common::OSUtilities::ILocalIP * Common::OSUtilities::localIP()
{
    return LocalIPStaticPointer().get();
}
