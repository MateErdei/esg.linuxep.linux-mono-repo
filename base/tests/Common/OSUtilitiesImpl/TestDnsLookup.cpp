/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockDnsLookup.h"
#include "MockILocalIP.h"

#include <Common/OSUtilities/IDnsLookup.h>
#include <Common/OSUtilitiesImpl/DnsLookupImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifndef ARTISANBUILD

using namespace Common::OSUtilities;

using PairResult = std::pair<std::string, std::string>;
using ListInputOutput = std::vector<PairResult>;

TEST(TestDnsLookup, shouldBeAbleToResolvValidHosts) // NOLINT
{
    auto dns = dnsLookup();
    ListInputOutput url_ip = { { "uk-filer6.eng.sophos", "10.101.200.100" },
                               { "uk-filer5.prod.sophos", "10.192.1.100" } };

    for (const auto& map : url_ip)
    {
        auto ips = dns->lookup(map.first);
        ASSERT_GT(ips.ip4collection.size(), 0);
        auto answer = ips.ip4collection.at(0).stringAddress();
        EXPECT_EQ(answer, map.second)
            << "ip of " << map.first << " differ. Expected: " << map.second << " but got: " << answer;
    }
}

TEST(TestDnsLookup, invalidRequestShouldThrow) // NOLINT
{
    auto dns = dnsLookup();
    std::vector<std::string> invalid_url = { { "nonuk-filer6.eng.sophos" }, { "this.server.does.not.exists" }

    };

    for (const auto& noserver : invalid_url)
    {
        EXPECT_THROW(dns->lookup(noserver), std::runtime_error); // NOLINT
    }
}

TEST(TestDnsLookup, shouldBeAbleToResolvValidIPv6) // NOLINT
{
    auto dns = dnsLookup();
    // the public url that I found that return ip6 and ip4
    auto ips = dns->lookup("www.google.com");
    ASSERT_GT(ips.ip4collection.size(), 0);
    ASSERT_GT(ips.ip6collection.size(), 0);
}
#endif

TEST(TestDnsLookup, canMockDns) // NOLINT
{
    std::unique_ptr<MockIDnsLookup> mockDNS(new StrictMock<MockIDnsLookup>());
    EXPECT_CALL(*mockDNS, lookup(_)).WillOnce(Return(MockILocalIP::buildIPsHelper("10.10.101.34")));
    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(mockDNS));

    auto ips = Common::OSUtilities::dnsLookup()->lookup("server.com");
    ASSERT_EQ(ips.ip4collection.size(), 1);
    EXPECT_EQ(ips.ip4collection.at(0).stringAddress(), "10.10.101.34");
    Common::OSUtilitiesImpl::restoreDnsLookup();
}

TEST(TestDnsLookup, canUsetheFakeDns) // NOLINT
{
    std::unique_ptr<FakeIDnsLookup> mockDNS(new StrictMock<FakeIDnsLookup>());
    mockDNS->addMap("server.com", { "10.10.101.34" });
    mockDNS->addMap("server2.com", { "10.10.101.35" });
    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(mockDNS));

    auto ips = Common::OSUtilities::dnsLookup()->lookup("server.com");
    ASSERT_EQ(ips.ip4collection.size(), 1);
    EXPECT_EQ(ips.ip4collection.at(0).stringAddress(), "10.10.101.34");

    ips = Common::OSUtilities::dnsLookup()->lookup("server2.com");
    ASSERT_EQ(ips.ip4collection.size(), 1);
    EXPECT_EQ(ips.ip4collection.at(0).stringAddress(), "10.10.101.35");

    EXPECT_THROW(Common::OSUtilities::dnsLookup()->lookup("no_server.com"), std::runtime_error); // NOLINT

    Common::OSUtilitiesImpl::restoreDnsLookup();
}
