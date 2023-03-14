// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "MockDnsLookup.h"
#include "MockILocalIP.h"

#include <Common/OSUtilities/IDnsLookup.h>
#include <Common/OSUtilitiesImpl/DnsLookupImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifndef ARTISANBUILD

using namespace Common::OSUtilities;

using PairResult = std::pair<std::string, std::vector<std::string>>;
using ListInputOutput = std::vector<PairResult>;

extern std::string ipsToString(const std::vector<std::string>& ips);

TEST(TestDnsLookup, DISABLED_shouldBeAbleToResolvValidHosts) // NOLINT
{
    auto dns = dnsLookup();
    ListInputOutput url_ip = {
        { "uk-filer6.eng.sophos", { "10.101.200.100", "100.78.0.19" } },
        { "uk-filer5.prod.sophos", { "10.192.1.100", "100.78.0.27" } }
    };

    for (const auto& map : url_ip)
    {
        auto ips = dns->lookup(map.first);
        ASSERT_GT(ips.ip4collection.size(), 0);
        auto answer = ips.ip4collection.at(0).stringAddress();
        auto it = std::find(map.second.begin(), map.second.end(), answer);
        ASSERT_NE(it, map.second.end())
            << "ip of " << map.first << " differ. Expected: " << ipsToString(map.second) << " but got: " << answer;
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

//Todo LINUXDAR-6988 replace test with a unit test using mock and real robot test
TEST(TestDnsLookup, DISABLED_shouldBeAbleToResolvValidIPv6) // NOLINT
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
