// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include <UpdateSchedulerImpl/UpdateSchedulerUtils.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <gtest/gtest.h>
#include <json.hpp>

class UpdateSchedulerUtils: public LogOffInitializedTests{};


TEST_F(UpdateSchedulerUtils, allHealthGoodWhenEndpointStateGood)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setInstallState("0");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 0;
    expectedHealth["installState"] = 0;
    expectedHealth["overall"] = 0;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}

TEST_F(UpdateSchedulerUtils, OverallHealthBadWhenInstallStateIsBad)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setInstallState("1");
    expectedStateMachineData.setDownloadFailedSinceTime("10");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 0;
    expectedHealth["installState"] = 1;
    expectedHealth["overall"] = 1;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}

TEST_F(UpdateSchedulerUtils, OverallHealthBadWhenDownloadStateIsBad)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("10");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 1;
    expectedHealth["installState"] = 0;
    expectedHealth["overall"] = 1;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}

TEST_F(UpdateSchedulerUtils, allHealthBadWhendownloadStateeIsGarbage)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("788");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("10");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 1;
    expectedHealth["installState"] = 0;
    expectedHealth["overall"] = 1;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}
TEST_F(UpdateSchedulerUtils, getJwTokenWhenConfigFileDoesNotExist)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();

    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    std::string token = UpdateSchedulerImpl::UpdateSchedulerUtils::getJWToken();
    ASSERT_EQ("",token);
}
TEST_F(UpdateSchedulerUtils, getJwTokenWhenConfigEmpty)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();

    std::vector<std::string> contents{{""}} ;

    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    std::string token = UpdateSchedulerImpl::UpdateSchedulerUtils::getJWToken();
    ASSERT_EQ("",token);
}
TEST_F(UpdateSchedulerUtils, getJwTokenFromFile)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();

    std::vector<std::string> contents{{"jwt_token=stuff"}} ;

    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));

    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    std::string token = UpdateSchedulerImpl::UpdateSchedulerUtils::getJWToken();
    ASSERT_EQ("stuff",token);
}

TEST_F(UpdateSchedulerUtils, getUpdateConfigWithLatestJWTWhenTokenInUpdateConfigIsUpToDate)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string mcsConfigFile = "/opt/sophos-spl/base/etc/sophosspl/mcs.config";
    std::vector<std::string> mcscontents{{"jwt_token=stuff"}} ;
    EXPECT_CALL(*filesystemMock, isFile(mcsConfigFile)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(mcsConfigFile)).Times(3).WillRepeatedly(Return(mcscontents));

    std::string updateConfigFile = "/opt/sophos-spl/base/update/var/updatescheduler/update_config.json";
    std::string updatecontents= R"({"JWToken": "stuff"})" ;
    EXPECT_CALL(*filesystemMock, isFile(updateConfigFile)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(updateConfigFile)).WillOnce(Return(updatecontents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::move(filesystemMock)};

    const auto[currentConfig, configUpdated] = UpdateSchedulerImpl::UpdateSchedulerUtils::getUpdateConfigWithLatestJWT();
    EXPECT_FALSE(configUpdated);
}

TEST_F(UpdateSchedulerUtils, getUpdateConfigWithLatestJWTWhenTokenInUpdateConfigIsOutOfDate)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    std::string mcsConfigFile = "/opt/sophos-spl/base/etc/sophosspl/mcs.config";
    std::vector<std::string> mcscontents{{"jwt_token=tuff","device_id=device","tenant_id=tenant"}} ;
    EXPECT_CALL(*filesystemMock, isFile(mcsConfigFile)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(mcsConfigFile)).Times(3).WillRepeatedly(Return(mcscontents));

    std::string updateConfigFile = "/opt/sophos-spl/base/update/var/updatescheduler/update_config.json";
    std::string updatecontents= "{\"JWToken\": \"stuff\", \"tenantId\": \"example-tenant-id\", \"deviceId\": \"example-device-id\"}" ;
    EXPECT_CALL(*filesystemMock, isFile(updateConfigFile)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(updateConfigFile)).WillOnce(Return(updatecontents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::move(filesystemMock)};

    const auto[currentConfig, configUpdated] = UpdateSchedulerImpl::UpdateSchedulerUtils::getUpdateConfigWithLatestJWT();
    ASSERT_TRUE(configUpdated);
    ASSERT_EQ(currentConfig.getJWToken(),"tuff");
    ASSERT_EQ(currentConfig.getDeviceId(),"device");
    ASSERT_EQ(currentConfig.getTenantId(),"tenant");
}

TEST_F(UpdateSchedulerUtils, getSuiteVersionNoFile)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(*filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath())).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::move(filesystemMock)};

    EXPECT_EQ(UpdateSchedulerImpl::UpdateSchedulerUtils::getSuiteVersion(), "");
}

class UpdateSchedulerUtilsParameterised
    : public ::testing::TestWithParam<std::pair<std::string, std::string>>
                   {};

INSTANTIATE_TEST_SUITE_P(
               UpdateSchedulerUtils,
               UpdateSchedulerUtilsParameterised,
                ::testing::Values(
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_2030.2000000000.10454546465654.1456456456453.951807f071.dat" />)", "2030.2000000000.10454546465654.1456456456453"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_2030.2.10.153.951807f071.dat" />)", "2030.2.10.153"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_2026.2.10.53.951807f071.dat" />)", "2026.2.10.53"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_2025.2.0.53.951807f071.dat" />)", "2025.2.0.53"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_2023.2.0.3.951807f071.dat" />)", "2023.2.0.3"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_2023.2.0.951807f071.dat" />)", "2023.2.0"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_951807f071.dat" />)", ""),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_.951807f071.dat" />)", ""),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base" />)", ""),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-Base_WHYAMIHERE.951807f071.dat" />)", "WHYAMIHERE"),
                           std::make_pair(R"(<suite ref="sdds3.ServerProtectionLinux-AV_2023.2.0.3.951807f071.dat" />)", "")));

TEST_P(UpdateSchedulerUtilsParameterised, SerialiseAndDeserialise)
{
    auto [input, output] = GetParam();
    std::vector<std::string> readLines {input};

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(*filesystemMock, exists(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath())).WillOnce(Return(readLines));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::move(filesystemMock)};

    EXPECT_EQ(UpdateSchedulerImpl::UpdateSchedulerUtils::getSuiteVersion(), output);
}


