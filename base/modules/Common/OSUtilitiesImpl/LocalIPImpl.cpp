// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "LocalIPImpl.h"

#include "Common/OSUtilities/IIPUtils.h"

#include <net/if.h>

#include <algorithm>
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

    void addIpToInterface(struct ifaddrs* listIFAddr, Common::OSUtilities::Interface& interface)
    {
        int family = listIFAddr->ifa_addr->sa_family;

        if (family == AF_INET)
        {
            struct sockaddr_in* ipSockAddr = reinterpret_cast<struct sockaddr_in*>(listIFAddr->ifa_addr);
            interface.ipAddresses.ip4collection.emplace_back(ipSockAddr);
        }
        else if (family == AF_INET6)
        {
            struct sockaddr_in6* ipSockAddr = reinterpret_cast<struct sockaddr_in6*>(listIFAddr->ifa_addr);
            interface.ipAddresses.ip6collection.emplace_back(ipSockAddr);
        }
    }

    std::vector<Common::OSUtilities::Interface> localInterfaces()
    {
        struct ifaddrs* listIFAddr;

        if (getifaddrs(&listIFAddr) == -1)
        {
            throw std::system_error(errno, std::generic_category(), "Failed to get local ip information.");
        }

        // ensure the entire list is cleared on return
        IfAddrScopeGuard ifAddrScopeGuard{ listIFAddr };
        std::vector<Common::OSUtilities::Interface> interfaces;

        // walk through the list
        for (; listIFAddr != nullptr; listIFAddr = listIFAddr->ifa_next)
        {
            if (listIFAddr->ifa_addr == nullptr)
            {
                continue;
            }

            if (listIFAddr->ifa_flags & IFF_LOOPBACK)
            {
                continue;
            }

            std::string interfaceName = listIFAddr->ifa_name;

            auto interfaceIter = std::find_if(
                interfaces.begin(),
                interfaces.end(),
                [&interfaceName](const Common::OSUtilities::Interface& interface)
                { return interface.name == interfaceName; });

            if (interfaceIter == interfaces.end()) // if interfaceName not found, create new interface
            {
                Common::OSUtilities::Interface interface;
                interface.name = interfaceName;
                addIpToInterface(listIFAddr, interface);

                interfaces.emplace_back(interface);
            }
            else
            {
                addIpToInterface(listIFAddr, *interfaceIter);
            }
        }
        return interfaces;
    }

    IPs localIPs()
    {
        std::vector<Common::OSUtilities::Interface> interfaces = localInterfaces();
        IPs ips;

        for (const auto& interface : interfaces)
        {
            ips.ip4collection.insert(
                ips.ip4collection.end(),
                interface.ipAddresses.ip4collection.begin(),
                interface.ipAddresses.ip4collection.end());
            ips.ip6collection.insert(
                ips.ip6collection.end(),
                interface.ipAddresses.ip6collection.begin(),
                interface.ipAddresses.ip6collection.end());
        }

        return ips;
    }

} // namespace

namespace Common::OSUtilitiesImpl
{
    Common::OSUtilities::IPs LocalIPImpl::getLocalIPs() const
    {
        return ::localIPs();
    }
    std::vector<Common::OSUtilities::Interface> LocalIPImpl::getLocalInterfaces() const
    {
        return ::localInterfaces();
    }

    void replaceLocalIP(ILocalIPPtr other)
    {
        LocalIPStaticPointer().reset(other.release());
    }

    void restoreLocalIP()
    {
        LocalIPStaticPointer().reset(new Common::OSUtilitiesImpl::LocalIPImpl());
    }
} // namespace Common::OSUtilitiesImpl

Common::OSUtilities::ILocalIP* Common::OSUtilities::localIP()
{
    return LocalIPStaticPointer().get();
}
