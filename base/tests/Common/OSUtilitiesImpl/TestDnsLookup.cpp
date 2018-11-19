/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilities/IDnsLookup.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Common::OSUtilities;

using PairResult = std::pair<std::string , std::string >;
using ListInputOutput = std::vector<PairResult >;


TEST(TestDnsLookup, shouldBeAbleToResolvValidHosts) // NOLINT
{
    auto dns = dnsLookup();
    ListInputOutput url_ip = {
            {"uk-filer6.eng.sophos", "10.101.200.100"},
            {"uk-filer5.prod.sophos", "10.192.1.100"}
    };

    for( const auto & map : url_ip )
    {
        auto ips = dns->lookup(map.first);
        ASSERT_GT(ips.ip4collection.size(), 0);
        auto answer = ips.ip4collection.at(0).stringAddress();
        EXPECT_EQ( answer , map.second) <<
            "ip of " << map.first << " differ. Expected: " << map.second << " but got: " << answer;
    }
}

TEST(TestDnsLookup, invalidRequestShouldThrow) // NOLINT
{
    auto dns = dnsLookup();
    std::vector<std::string> invalid_url = {
            {"nonuk-filer6.eng.sophos"},
            {"this.server.does.not.exists"}

    };

    for( const auto & noserver: invalid_url )
    {
        EXPECT_THROW(dns->lookup(noserver), std::runtime_error);
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