// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "MockDnsLookup.h"
#include "MockILocalIP.h"

#include "Common/OSUtilities/IDnsLookup.h"
#include "Common/OSUtilitiesImpl/DnsLookupImpl.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockSysCalls.h"

#include <arpa/inet.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifndef ARTISANBUILD

using namespace Common::OSUtilities;

using PairResult = std::pair<std::string, std::vector<std::string>>;
using ListInputOutput = std::vector<PairResult>;

std::string ipsToString(const std::vector<std::string>& ips)
{
    std::ostringstream os;
    bool first = true;

    for (const auto& ip : ips)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            os << " ";
        }
        os << ip;
    }

    return os.str();
}

class TestDnsLookup: public LogOffInitializedTests
{
public:
    void SetUp()
    {
        m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    }
    
    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
};

TEST_F(TestDnsLookup, shouldBeAbleToResolvValidHosts)
{
    auto dns = dnsLookup();
    ListInputOutput url_ip = {
        { "uk-filer6.eng.sophos", { "10.101.200.100", "100.78.0.19" } },
        { "uk-filer5.prod.sophos", { "10.192.1.100", "100.78.0.27" } }
    };

    for (const auto& map : url_ip)
    {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(map.second[0].c_str());
        addrinfo ifa{};
        ifa.ai_addr = (struct sockaddr*)&address;
        ifa.ai_addrlen = sizeof (struct sockaddr_in);
        EXPECT_CALL(*m_mockSysCalls, getaddrinfo(_, _, _, _)).WillOnce(DoAll(SetArgPointee<3>(&ifa), Return(0)));
        EXPECT_CALL(*m_mockSysCalls, freeaddrinfo(_));
        
        auto ips = dns->lookup(map.first, m_mockSysCalls);
        ASSERT_GT(ips.ip4collection.size(), 0);
        auto answer = ips.ip4collection.at(0).stringAddress();
        auto it = std::find(map.second.begin(), map.second.end(), answer);
        ASSERT_NE(it, map.second.end())
            << "ip of " << map.first << " differ. Expected: " << ipsToString(map.second) << " but got: " << answer;
    }
}

TEST_F(TestDnsLookup, invalidRequestShouldThrow)
{
    auto dns = dnsLookup();
    std::string invalid_url = "this.server.does.not.exists";
    EXPECT_CALL(*m_mockSysCalls, getaddrinfo(invalid_url.c_str(), _, _, _)).WillOnce(Return(EAI_AGAIN));
    EXPECT_THROW(dns->lookup(invalid_url, m_mockSysCalls), std::runtime_error);
}

TEST_F(TestDnsLookup, shouldBeAbleToResolvValidIPv6)
{
    sockaddr_in address1{};
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = inet_addr("142.250.187.196");
    addrinfo ifa1{};
    ifa1.ai_addr = (struct sockaddr*)&address1;
    ifa1.ai_addrlen = sizeof (struct sockaddr_in);
    sockaddr_in6 address2{};
    address2.sin6_family = AF_INET6;
    address2.sin6_addr = in6_addr();
    addrinfo ifa2{};
    ifa2.ai_addr = (struct sockaddr*)&address2;
    ifa2.ai_addrlen = sizeof (struct sockaddr_in);
    ifa2.ai_next = &ifa1;
    EXPECT_CALL(*m_mockSysCalls, getaddrinfo(_, _, _, _)).WillOnce(DoAll(SetArgPointee<3>(&ifa2), Return(0)));
    EXPECT_CALL(*m_mockSysCalls, freeaddrinfo(_));
    
    auto dns = dnsLookup();
    auto ips = dns->lookup("www.google.com", m_mockSysCalls);
    ASSERT_GT(ips.ip4collection.size(), 0);
    ASSERT_GT(ips.ip6collection.size(), 0);
}
#endif

TEST_F(TestDnsLookup, canMockDns)
{
    std::unique_ptr<StrictMock<MockIDnsLookup>> mockDNS(new StrictMock<MockIDnsLookup>());
    EXPECT_CALL(*mockDNS, lookup(_, _)).WillOnce(Return(MockILocalIP::buildIPsHelper("10.10.101.34")));
    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(mockDNS));

    auto ips = Common::OSUtilities::dnsLookup()->lookup("server.com", m_mockSysCalls);
    ASSERT_EQ(ips.ip4collection.size(), 1);
    EXPECT_EQ(ips.ip4collection.at(0).stringAddress(), "10.10.101.34");
    Common::OSUtilitiesImpl::restoreDnsLookup();
}

TEST_F(TestDnsLookup, canUsetheFakeDns)
{
    std::unique_ptr<FakeIDnsLookup> mockDNS(new StrictMock<FakeIDnsLookup>());
    mockDNS->addMap("server.com", { "10.10.101.34" });
    mockDNS->addMap("server2.com", { "10.10.101.35" });
    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(mockDNS));

    auto ips = Common::OSUtilities::dnsLookup()->lookup("server.com", m_mockSysCalls);
    ASSERT_EQ(ips.ip4collection.size(), 1);
    EXPECT_EQ(ips.ip4collection.at(0).stringAddress(), "10.10.101.34");

    ips = Common::OSUtilities::dnsLookup()->lookup("server2.com", m_mockSysCalls);
    ASSERT_EQ(ips.ip4collection.size(), 1);
    EXPECT_EQ(ips.ip4collection.at(0).stringAddress(), "10.10.101.35");

    EXPECT_THROW(Common::OSUtilities::dnsLookup()->lookup("no_server.com", m_mockSysCalls), std::runtime_error);

    Common::OSUtilitiesImpl::restoreDnsLookup();
}
