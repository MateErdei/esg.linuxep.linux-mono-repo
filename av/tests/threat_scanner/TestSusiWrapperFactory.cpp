/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../common/LogInitializedTests.h"

// Include the .cpp to get static functions...
#include "sophos_threat_detector/threat_scanner/SusiWrapperFactory.cpp" // NOLINT

#include <gtest/gtest.h>

#define BASE "/tmp/TestSusiWrapperFactory/chroot/"
using namespace threat_scanner;

namespace
{
    class TestSusiWrapperFactory : public LogInitializedTests
    {};
}

void setupFilesForTestingGlobalRep()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("SOPHOS_INSTALL", BASE);
    fs::path PLUGIN_INSTALL = BASE;
    PLUGIN_INSTALL /= "plugins/av";
    appConfig.setData("PLUGIN_INSTALL", PLUGIN_INSTALL);

    fs::path fakeEtcDirectory  = BASE;
    fakeEtcDirectory  /= "base/etc";
    fs::path machineIdFilePath = fakeEtcDirectory;
    machineIdFilePath /= "machine_id.txt";

    fs::path customerIdFilePath = PLUGIN_INSTALL;
    customerIdFilePath /= "var/customer_id.txt";
    auto varDirectory = customerIdFilePath.parent_path();

    fs::create_directories("tmp/TestSusiWrapperFactory");
    fs::create_directories(BASE);
    fs::create_directories(fakeEtcDirectory);
    fs::create_directories(varDirectory);

    std::ofstream machineIdFile(machineIdFilePath);
    ASSERT_TRUE(machineIdFile.good());
    machineIdFile << "ab7b6758a3ab11ba8a51d25aa06d1cf4";
    machineIdFile.close();

    std::ofstream customerIdFileStream(customerIdFilePath);
    ASSERT_TRUE(customerIdFileStream.good());
    customerIdFileStream << "d22829d94b76c016ec4e04b08baeffaa";
    customerIdFileStream.close();
}

TEST(TestSusiWrapperFactory, getCustomerIdReturnsUnknown) // NOLINT
{
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST(TestSusiWrapperFactory, getEndpointIdReturnsUnknown) // NOLINT
{
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST(TestSusiWrapperFactory, getEndpointIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getEndpointId(),"ab7b6758a3ab11ba8a51d25aa06d1cf4");
}

TEST(TestSusiWrapperFactory, geCustomerIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getCustomerId(),"d22829d94b76c016ec4e04b08baeffaa");
}