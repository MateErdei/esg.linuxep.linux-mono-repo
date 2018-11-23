/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/OSUtilities/ILocalIP.h>
#include <Common/OSUtilitiesImpl/LocalIPImpl.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/Process/IProcess.h>
#include <Common/FileSystem/IFileSystem.h>
#include "MockILocalIP.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Common::OSUtilities;

using PairResult = std::pair<std::string , std::string >;
using ListInputOutput = std::vector<PairResult >;


TEST(TestLocalIP, shouldBeAbleToResolvValidHosts) // NOLINT
{
    auto fSystem = Common::FileSystem::fileSystem();
    std::string ipconfigInfo = "/sbin/ifconfig";
    std::vector<std::string> arguments;

    if (!fSystem->isExecutable(ipconfigInfo))
    {
        ipconfigInfo = "/usr/sbin/ip";
        arguments.push_back("address");
        if (!fSystem->isExecutable(ipconfigInfo))
        {

            std::cout << "[  SKIPPED ] /sbin/ifconfig or /usr/sbin/ip not present " << std::endl;
            return;
        }
    }
    auto process = Common::Process::createProcess();
    process->exec(ipconfigInfo, arguments);
    std::string ifconfigOutput = process->output();

    auto localips = localIP()->getLocalIPs();
    EXPECT_GT(localips.ip4collection.size(), 0);
    for( auto & ip4 : localips.ip4collection)
    {
        std::string fullipword = ip4.stringAddress();
        EXPECT_GT( fullipword.size(), 6); 
        EXPECT_THAT(ifconfigOutput, ::testing::HasSubstr(fullipword));
    }

    // it is not required to have ipv6 in our servers, but if they exists, check they are valid.
    for( auto & ip6 : localips.ip6collection)
    {
        std::string ip6String = ip6.stringAddress();
        auto findPercentage = ip6String.find("%");
        if ( findPercentage != std::string::npos)
        {
            std::string onlyip6 = ip6String.substr(0, findPercentage);
            ip6String = onlyip6;
        }
        std::string fullipword = " " + ip6String + " ";
        EXPECT_THAT(ifconfigOutput, ::testing::HasSubstr(fullipword));
    }
}

TEST(TestLocalIP, canMockLocalIPs) // NOLINT
{
    std::unique_ptr<MockILocalIP> mocklocalIP (new StrictMock<MockILocalIP>());
    EXPECT_CALL(*mocklocalIP, getLocalIPs()).WillOnce(Return(MockILocalIP::buildIPsHelper("10.10.101.34")));
    Common::OSUtilitiesImpl::replaceLocalIP(std::move( mocklocalIP));

    auto ips = Common::OSUtilities::localIP()->getLocalIPs();
    ASSERT_EQ( ips.ip4collection.size(), 1);
    EXPECT_EQ( ips.ip4collection.at(0).stringAddress(), "10.10.101.34");
    Common::OSUtilitiesImpl::restoreLocalIP();

}


TEST(TestLocalIP, canUsetheFakeLocalIPs) // NOLINT
{
    std::unique_ptr<FakeILocalIP> fakeILocalIP(new FakeILocalIP(std::vector<std::string>{"10.10.101.34", "10.10.101.35"}));
    Common::OSUtilitiesImpl::replaceLocalIP( std::move(fakeILocalIP));

    auto ips = Common::OSUtilities::localIP()->getLocalIPs();
    ASSERT_EQ( ips.ip4collection.size(), 2);
    EXPECT_EQ( ips.ip4collection.at(0).stringAddress(), "10.10.101.34");
    EXPECT_EQ( ips.ip4collection.at(1).stringAddress(), "10.10.101.35");
    Common::OSUtilitiesImpl::restoreLocalIP();

}