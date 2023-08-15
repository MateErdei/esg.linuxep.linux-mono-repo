// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "SulDownloader/sdds3/SDDS3Repository.h"
#include "SulDownloader/sdds3/SDDS3Utils.h"
#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
#include "sophlib/sdds3/PackageRef.h"

#include <gtest/gtest.h>

using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class Sdds3UtilsTest : public ::testing::Test {};

TEST_F(Sdds3UtilsTest, createSUSRequestBodyCreatesExpectedBodyForNoFixedVersionAndNoESM)
{
    SUSRequestParameters requestParameters;
    requestParameters.product = "linuxep";
    requestParameters.platformToken = "LINUX_INTEL_LIBC6";
    std::vector<Common::Policy::ProductSubscription> subscriptions;
    subscriptions.emplace_back("rigidname", "", "RECOMMENDED", "");

    requestParameters.subscriptions = subscriptions;
    std::string requestBody = createSUSRequestBody(requestParameters);
    EXPECT_EQ(requestBody, R"({"platform_token":"LINUX_INTEL_LIBC6","product":"linuxep","schema_version":1,"server":false,"subscriptions":[{"id":"rigidname","tag":"RECOMMENDED"}]})");
}

TEST_F(Sdds3UtilsTest, createSUSRequestBodyCreatesExpectedBodyForFixedVersionWithNoESM)
{
    SUSRequestParameters requestParameters;
    requestParameters.product = "linuxep";
    requestParameters.platformToken = "LINUX_INTEL_LIBC6";
    std::vector<Common::Policy::ProductSubscription> subscriptions;
    subscriptions.emplace_back("rigidname", "", "RECOMMENDED", "1.0.0");

    requestParameters.subscriptions = subscriptions;
    std::string requestBody = createSUSRequestBody(requestParameters);
    EXPECT_EQ(requestBody, R"({"platform_token":"LINUX_INTEL_LIBC6","product":"linuxep","schema_version":1,"server":false,"subscriptions":[{"fixedVersion":"1.0.0","id":"rigidname","tag":"RECOMMENDED"}]})");
}

TEST_F(Sdds3UtilsTest, createSUSRequestBodyCreatesExpectedBodyForFixedVersionWithESM)
{
    SUSRequestParameters requestParameters;
    requestParameters.product = "linuxep";
    requestParameters.platformToken = "LINUX_INTEL_LIBC6";

    std::vector<Common::Policy::ProductSubscription> subscriptions;
    subscriptions.emplace_back("rigidname", "", "RECOMMENDED", "1.0.0");
    requestParameters.subscriptions = subscriptions;

    Common::Policy::ESMVersion esmVersion("name", "token");
    requestParameters.esmVersion = std::move(esmVersion);

    std::string requestBody = createSUSRequestBody(requestParameters);
    EXPECT_EQ(requestBody, R"({"fixed_version_token":"token","platform_token":"LINUX_INTEL_LIBC6","product":"linuxep","schema_version":1,"server":false,"subscriptions":[{"fixedVersion":"1.0.0","id":"rigidname","tag":"RECOMMENDED"}]})");
}

TEST_F(Sdds3UtilsTest, createSUSRequestBodyCreatesExpectedBodyForNoFixedVersionWithESM)
{
    SUSRequestParameters requestParameters;
    requestParameters.product = "linuxep";
    requestParameters.platformToken = "LINUX_INTEL_LIBC6";

    std::vector<Common::Policy::ProductSubscription> subscriptions;
    subscriptions.emplace_back("rigidname", "", "RECOMMENDED", "");
    requestParameters.subscriptions = subscriptions;

    Common::Policy::ESMVersion esmVersion("name", "token");
    requestParameters.esmVersion = std::move(esmVersion);

    std::string requestBody = createSUSRequestBody(requestParameters);
    EXPECT_EQ(requestBody, R"({"fixed_version_token":"token","platform_token":"LINUX_INTEL_LIBC6","product":"linuxep","schema_version":1,"server":false,"subscriptions":[{"id":"rigidname","tag":"RECOMMENDED"}]})");
}
