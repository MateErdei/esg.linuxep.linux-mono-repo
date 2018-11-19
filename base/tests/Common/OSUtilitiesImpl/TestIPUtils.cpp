/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilities/IDnsLookup.h>
#include <Common/OSUtilities/IIPUtils.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <tuple>

#include <arpa/inet.h>
using namespace Common::OSUtilities;

using PairResult = std::pair<std::string , std::string >;
using ListInputOutput = std::vector<PairResult >;


TEST(TestIPUtils, verifyIP4CanbeconstructedByString) // NOLINT
{
    auto dns = dnsLookup();
    std::vector<std::pair<std::string, std::string>> url_ip = {
            {"uk-filer6.eng.sophos", "10.101.200.100"},
            {"uk-filer5.prod.sophos", "10.192.1.100"}
    };

    for( const auto & map : url_ip )
    {
        auto ips = dns->lookup(map.first);
        auto answerip = ips.ip4collection.at(0);

        IP4 expected(map.second);

        EXPECT_EQ( expected.stringAddress(), answerip.stringAddress());
        EXPECT_EQ( expected.ipAddress(), answerip.ipAddress()) << std::hex << expected.ipAddress() << " " << answerip.ipAddress() << std::dec;
    }
}


TEST(TestIPUtils, IP4CannotBeConstructedByInvalidIPString) // NOLINT
{
    std::vector<std::string> invalid_ips = {
            "270.101.89.90", // each number must be smaller than 256
            "10.101.89.90.89", // there are only 4 fields
    };
    for( auto invalid_ip: invalid_ips)
    {
        EXPECT_THROW(IP4{invalid_ip}, std::runtime_error) << "Did not rejected invalid ip: " << invalid_ip ;
    }
}

TEST(TestIPUtils, IP4distance) // NOLINT
{
    std::vector<std::tuple<std::string, std::string, int>> ip_distances = {
            {"255.0.0.0","255.0.0.0", 0}, //oxff oxff
            {"255.0.0.0","127.0.0.0", 32}, // oxff ox7f
            {"255.0.0.0","128.0.0.0", 31}, // oxff ox80
            {"127.0.0.0","64.0.0.0", 31},  // ox7f ox40
            {"127.0.0.0","63.0.0.0", 30},  // ox7f ox3f
            {"3.0.0.0","1.0.0.0", 25},  // ox7f ox3f
            {"1.255.0.0","0.255.0.0", 25},
            {"0.255.0.5","0.255.0.4", 1}
    };

    for( auto & pairentry: ip_distances)
    {
        IP4 ip1(std::get<0>(pairentry));
        IP4 ip2(std::get<1>(pairentry));
        EXPECT_EQ( ip1.distance(ip2),std::get<2>(pairentry) ) << "Input: " << ip1.stringAddress() << ",  " << ip2.stringAddress() ;
    }
}
