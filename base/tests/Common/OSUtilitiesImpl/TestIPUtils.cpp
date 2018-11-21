/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilities/IDnsLookup.h>
#include <Common/OSUtilities/IIPUtils.h>
#include <Common/OSUtilitiesImpl/DnsLookupImpl.h>
#include <Common/OSUtilitiesImpl/LocalIPImpl.h>
#include "MockILocalIP.h"
#include "MockDnsLookup.h"
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
            {"127.0.0.0","64.0.0.0", 30},  // ox7f ox40
            {"127.0.0.0","63.0.0.0", 31},  // ox7f ox3f
            {"3.0.0.0","1.0.0.0", 26},  // ox7f ox3f
            {"1.255.0.0","0.255.0.0", 25},
            {"0.255.0.5","0.255.0.4", 1},
            {"0.255.0.1","0.255.0.0", 1},
            {"0.255.0.2","0.255.0.1", 2},
            {"0.255.0.2","0.255.0.0", 2},
            {"0.255.0.4","0.255.0.3", 3},
            {"0.255.0.8","0.255.0.7", 4}
    };

    for( auto & pairentry: ip_distances)
    {
        IP4 ip1(std::get<0>(pairentry));
        IP4 ip2(std::get<1>(pairentry));
        EXPECT_EQ( ip1.distance(ip2),std::get<2>(pairentry) ) << "Input: " << ip1.stringAddress() << ",  " << ip2.stringAddress() ;
    }
}


TEST(TestIPUtils, extractServerFromHttpURL) // NOLINT
{
    std::vector<std::pair<std::string, std::string>> httpurl_server = {
            {"maineng2.eng.sophos:8191","maineng2.eng.sophos"},
            {"http://maineng2.eng.sophos:8191","maineng2.eng.sophos"},
            {"https://maineng2.eng.sophos:8191","maineng2.eng.sophos"},
            {"https://maineng2.eng.sophos:80","maineng2.eng.sophos"},
            {"https://maineng2.eng.sophos","maineng2.eng.sophos"},
            {"http://maineng2.eng.sophos","maineng2.eng.sophos"},
            {"http://maineng2.eng.sophos/path/to/app","maineng2.eng.sophos"},
            {"http://maineng2.eng.sophos:70/path/to/app","maineng2.eng.sophos"},
            {"maineng2.eng.sophos","maineng2.eng.sophos"},
            {"notEvenAValidURL","notEvenAValidURL"}
    };

    for( auto & pairentry: httpurl_server)
    {
        EXPECT_EQ(  tryExtractServerFromHttpURL(pairentry.first), pairentry.second )  << "Input: " <<  pairentry.first ;
    }
}



class TestSortServers : public ::testing::Test
{
public:
    FakeIDnsLookup * fakeDNS;
    TestSortServers()
    {
        std::unique_ptr<FakeILocalIP> fakeILocalIP(new FakeILocalIP(std::vector<std::string>{"192.168.10.5", "10.10.5.6"}));
        Common::OSUtilitiesImpl::replaceLocalIP( std::move(fakeILocalIP));
        std::unique_ptr<FakeIDnsLookup> fake(new FakeIDnsLookup());
        fakeDNS = fake.get();
        Common::OSUtilitiesImpl::replaceDnsLookup( std::move(fake));

    }

    void setupServers( std::vector<std::pair<std::string, std::vector<std::string>>> mapServerToAddressCollection)
    {
        for( auto & server2address : mapServerToAddressCollection)
        {
            fakeDNS->addMap(server2address.first, server2address.second);
        }
    }

    ~TestSortServers()
    {
        Common::OSUtilitiesImpl::restoreLocalIP();
        Common::OSUtilitiesImpl::restoreDnsLookup();
    }
};




TEST_F(TestSortServers, ifNoDNSIsKnownTheOrderDoesNotChange) // NOLINT
{
    auto report = indexOfSortedURIsByIPProximity({"url1", "url2", "url3"});
    std::vector<int> expectedValues{0,1,2};
    ASSERT_EQ( sortedIndexes(report), expectedValues);
}

TEST_F(TestSortServers, ifOnlyOneIsKnownItBecomeTheFirst) // NOLINT
{
    setupServers( {
      {"url2", {"201.20.5.6"}}
    });
    auto report = indexOfSortedURIsByIPProximity({"url1", "url2", "url3"});
    std::vector<int> expectedValues{1, 0,2};
    ASSERT_EQ( sortedIndexes(report), expectedValues);
}

TEST_F(TestSortServers, theEntriesAreSortedByIPProximity) // NOLINT
{
    // closer to 10.10.5.6
    setupServers( {
                          {"url1", {"10.6.5.6"}},
                          {"url2", {"10.10.100.6"}},
                          {"url3", {"10.10.5.7"}}

                  });
    auto report = indexOfSortedURIsByIPProximity({"url1", "url2", "url3"});
    std::vector<int> expectedValues{2, 1,0};
    ASSERT_EQ( sortedIndexes(report), expectedValues);
}


TEST_F(TestSortServers, theEntriesConsiderAllTheInterfaces) // NOLINT
{
    // closer to 192.168.10.5
    setupServers( {
                          {"url1", {"192.168.150.5"}},
                          {"url2", {"192.168.10.200"}},
                          {"url3", {"192.1.10.5"}}
                  });
    auto report = indexOfSortedURIsByIPProximity({"url1", "url2", "url3"});
    std::vector<int> expectedValues{1, 0,2};
    ASSERT_EQ( sortedIndexes(report), expectedValues);
}


TEST_F(TestSortServers, theEntriesConsiderAllTheAvailableDNSEntries) // NOLINT
{
    // closer to 192.168.10.5
    setupServers( {
                          {"url1", {"192.168.150.5", "10.1.5.6"}},
                          {"url2", {"192.168.10.200", "200.8.9.10"} },
                          {"url3", {"192.1.10.5", "8.10.5.6"}}
                  });
    auto report = indexOfSortedURIsByIPProximity({"url1", "url2", "url3"});
    std::vector<int> expectedValues{1, 0,2};
    ASSERT_EQ( sortedIndexes(report), expectedValues);
}
