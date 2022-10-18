/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/ILocalIP.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockILocalIP : public Common::OSUtilities::ILocalIP
{
public:
    static Common::OSUtilities::IPs buildIPsHelper(const std::string& ip4string)
    {
        Common::OSUtilities::IPs ips;
        ips.ip4collection.emplace_back(ip4string);
        return ips;
    }
    static std::vector<Common::OSUtilities::Interface> buildInterfacesHelper()
    {
        Common::OSUtilities::Interface interface1;
        interface1.ipAddresses.ip4collection.emplace_back("100.80.13.214");
        interface1.ipAddresses.ip6collection.emplace_back("8cc1:bec7:87c5:b668:62bf:ccaa:9f56:8f7f");
        interface1.ipAddresses.ip6collection.emplace_back("edbd:3bb8:bffb:4b6b:3536:2be7:3066:4276");
        interface1.name = "docker0";

        Common::OSUtilities::Interface interface2;
        interface2.ipAddresses.ip4collection.emplace_back("125.184.182.125");
        interface2.ipAddresses.ip6collection.emplace_back("4d48:b9eb:62a5:4119:6e7f:89e9:a652:4941");
        interface2.name = "em0";

        Common::OSUtilities::Interface interface3;
        interface3.ipAddresses.ip4collection.emplace_back("192.168.168.64");
        interface3.ipAddresses.ip6collection.emplace_back("f5eb:f7a4:323f:e898:f212:d0a6:3405:feec");
        interface3.name = "eth0";

        Common::OSUtilities::Interface interface4;
        interface4.ipAddresses.ip4collection.emplace_back("192.240.35.35");
        interface4.ipAddresses.ip6collection.emplace_back("e36:e861:82a2:a473:2c24:7a07:7867:6a50");
        interface4.name = "em1";

        Common::OSUtilities::Interface interface5;
        interface5.ipAddresses.ip4collection.emplace_back("172.6.251.34");
        interface5.name = "bond0";

        std::vector<Common::OSUtilities::Interface> interfaces = {interface1, interface2, interface3, interface4, interface5};

        return interfaces;
    }
    MOCK_CONST_METHOD0(getLocalIPs, Common::OSUtilities::IPs());
    MOCK_CONST_METHOD0(getLocalInterfaces, std::vector<Common::OSUtilities::Interface>());
};

class FakeILocalIP : public Common::OSUtilities::ILocalIP
{
    Common::OSUtilities::IPs m_ips;

public:
    FakeILocalIP(const std::vector<std::string>& ip4s, const std::vector<std::string>& ip6s)
    {
        for (auto& ip4 : ip4s)
        {
            m_ips.ip4collection.emplace_back(Common::OSUtilities::IP4{ ip4 });
        }
        for (auto& ip6 : ip6s)
        {
            m_ips.ip6collection.emplace_back(Common::OSUtilities::IP6{ ip6 });
        }
    }

    FakeILocalIP(const std::vector<std::string>& ip4s) : FakeILocalIP(ip4s, {}) {}

    Common::OSUtilities::IPs getLocalIPs() const { return m_ips; }
    std::vector<Common::OSUtilities::Interface> getLocalInterfaces() const { return {}; }
};
