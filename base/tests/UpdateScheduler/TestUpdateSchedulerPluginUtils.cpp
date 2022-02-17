/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <UpdateSchedulerImpl/UpdateSchedulerUtils.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
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
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string mcsConfigFile = "/opt/sophos-spl/base/etc/sophosspl/mcs.config";
    std::vector<std::string> mcscontents{{"jwt_token=stuff"}} ;
    EXPECT_CALL(*filesystemMock, isFile(mcsConfigFile)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(mcsConfigFile)).WillOnce(Return(mcscontents));

    std::string updateConfigFile = "/opt/sophos-spl/base/update/var/updatescheduler/update_config.json";
    std::string updatecontents= "{\"JWToken\": \"stuff\"}" ;
    EXPECT_CALL(*filesystemMock, isFile(updateConfigFile)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(updateConfigFile)).WillOnce(Return(updatecontents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> pair = UpdateSchedulerImpl::UpdateSchedulerUtils::getUpdateConfigWithLatestJWT();
    ASSERT_EQ(pair.second,false);
}

TEST_F(UpdateSchedulerUtils, getUpdateConfigWithLatestJWTWhenTokenInUpdateConfigIsOutOfDate)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string mcsConfigFile = "/opt/sophos-spl/base/etc/sophosspl/mcs.config";
    std::vector<std::string> mcscontents{{"jwt_token=tuff"}} ;
    EXPECT_CALL(*filesystemMock, isFile(mcsConfigFile)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines(mcsConfigFile)).WillOnce(Return(mcscontents));

    std::string updateConfigFile = "/opt/sophos-spl/base/update/var/updatescheduler/update_config.json";
    std::string updatecontents= "{\"JWToken\": \"stuff\"}" ;
    EXPECT_CALL(*filesystemMock, isFile(updateConfigFile)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(updateConfigFile)).WillOnce(Return(updatecontents));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> pair = UpdateSchedulerImpl::UpdateSchedulerUtils::getUpdateConfigWithLatestJWT();
    ASSERT_EQ(pair.second,true);
    ASSERT_EQ(pair.first.getJWToken(),"tuff");
}