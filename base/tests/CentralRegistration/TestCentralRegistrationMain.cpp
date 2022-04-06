/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <CentralRegistration/Main.h>
#include <cmcsrouter/Config.h>
#include <cmcsrouter/ConfigOptions.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/MockSystemUtils.h>

class CentralRegistrationMainTests : public LogInitializedTests
{

};

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArguments) // NOLINT
{
    std::vector<std::string> argValues{
        "CentralRegistration", "MCS_Token001", "https://MCS_URL", "--groups=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    MCS::ConfigOptions configOptions = CentralRegistrationImpl::processCommandLineOptions(argValues, mockSystemUtils);

    EXPECT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[1]);
    EXPECT_EQ(configOptions.config[MCS::MCS_URL], argValues[2]);
    EXPECT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    EXPECT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_CERT],"");

}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithProxyCreds) // NOLINT
{
    std::vector<std::string> argValues{
        "CentralRegistration", "MCS_Token001", "https://MCS_URL", "--groups=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return("user:password"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    MCS::ConfigOptions configOptions = CentralRegistrationImpl::processCommandLineOptions(argValues, mockSystemUtils);

    EXPECT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[1]);
    EXPECT_EQ(configOptions.config[MCS::MCS_URL], argValues[2]);
    EXPECT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    EXPECT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME],"user");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD],"password");
    EXPECT_EQ(configOptions.config[MCS::MCS_CERT],"");

}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithMCS_CA_Override) // NOLINT
{
    std::vector<std::string> argValues{
        "CentralRegistration", "MCS_Token001", "https://MCS_URL", "--groups=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return("some_path"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    MCS::ConfigOptions configOptions = CentralRegistrationImpl::processCommandLineOptions(argValues, mockSystemUtils);

    EXPECT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[1]);
    EXPECT_EQ(configOptions.config[MCS::MCS_URL], argValues[2]);
    EXPECT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    EXPECT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_CA_OVERRIDE],"some_path");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithHTTPSEnvProxySet) // NOLINT
{
    std::vector<std::string> argValues{
        "CentralRegistration", "MCS_Token001", "https://MCS_URL", "--groups=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return("https://secure_proxy:443"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).Times(0);

    MCS::ConfigOptions configOptions = CentralRegistrationImpl::processCommandLineOptions(argValues, mockSystemUtils);

    EXPECT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[1]);
    EXPECT_EQ(configOptions.config[MCS::MCS_URL], argValues[2]);
    EXPECT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    EXPECT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY],"https://secure_proxy:443");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_CERT],"");

}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithHTTPEnvProxySet) // NOLINT
{
    std::vector<std::string> argValues{
        "CentralRegistration", "MCS_Token001", "https://MCS_URL", "--groups=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return("http://non_secure_proxy:80"));

    MCS::ConfigOptions configOptions = CentralRegistrationImpl::processCommandLineOptions(argValues, mockSystemUtils);

    EXPECT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[1]);
    EXPECT_EQ(configOptions.config[MCS::MCS_URL], argValues[2]);
    EXPECT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    EXPECT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY],"http://non_secure_proxy:80");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD],"");
    EXPECT_EQ(configOptions.config[MCS::MCS_CERT],"");

}