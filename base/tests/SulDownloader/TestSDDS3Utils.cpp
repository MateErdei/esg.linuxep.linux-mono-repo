/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <modules/SulDownloader/sdds3/SDDS3Utils.h>
#include <modules/SulDownloader/sdds3/SDDS3Repository.h>
#include <modules/SulDownloader/suldownloaderdata/ConfigurationData.h>

#include <PackageRef.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class Sdds3UtilsTest : public ::testing::Test {};

TEST_F(Sdds3UtilsTest, createSUSRequestBodyCreatesExpectedBodyForNoFixedVersion) // NOLINT
{
    SUSRequestParameters requestParameters;
    requestParameters.product = "linuxep";
    requestParameters.platformToken = "LINUX_INTEL_LIBC6";
    std::vector<suldownloaderdata::ProductSubscription> subscriptions;
    subscriptions.emplace_back("rigidname", "", "RECOMMENDED", "");

    requestParameters.subscriptions = subscriptions;
    std::string requestBody = createSUSRequestBody(requestParameters);
    EXPECT_EQ(requestBody, "{\"platform_token\":\"LINUX_INTEL_LIBC6\",\"product\":\"linuxep\",\"schema_version\":1,\"server\":false,\"subscriptions\":[{\"id\":\"rigidname\",\"tag\":\"RECOMMENDED\"}]}");
}

TEST_F(Sdds3UtilsTest, createSUSRequestBodyCreatesExpectedBodyForFixedVersion) // NOLINT
{
    SUSRequestParameters requestParameters;
    requestParameters.product = "linuxep";
    requestParameters.platformToken = "LINUX_INTEL_LIBC6";
    std::vector<suldownloaderdata::ProductSubscription> subscriptions;
    subscriptions.emplace_back("rigidname", "", "RECOMMENDED", "1.0.0");

    requestParameters.subscriptions = subscriptions;
    std::string requestBody = createSUSRequestBody(requestParameters);
    EXPECT_EQ(requestBody, "{\"platform_token\":\"LINUX_INTEL_LIBC6\",\"product\":\"linuxep\",\"schema_version\":1,\"server\":false,\"subscriptions\":[{\"fixedVersion\":\"1.0.0\",\"id\":\"rigidname\",\"tag\":\"RECOMMENDED\"}]}");
}

