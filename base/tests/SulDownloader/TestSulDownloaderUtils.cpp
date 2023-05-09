// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/SulDownloader/SulDownloaderUtils.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <gtest/gtest.h>

using namespace SulDownloader::suldownloaderdata;

class SulDownloaderUtilsTest : public ::testing::Test {};

class SulDownloaderUtilsTestFiles : public LogInitializedTests
{
    void SetUp() override
    {
        m_fileSystemMock = new NiceMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_fileSystemMock));
    }

public:
    SulDownloaderUtilsTestFiles() : m_fileSystemMock(nullptr) {}

    MockFileSystem* m_fileSystemMock;

    Tests::ScopedReplaceFileSystem m_replacer;
    std::string markerFile =
        Common::ApplicationConfiguration::applicationPathManager().getForcedAnUpdateMarkerPath();
    std::string pausedMarkerFile = Common::ApplicationConfiguration::applicationPathManager().getForcedAPausedUpdateMarkerPath();
};

TEST_F(SulDownloaderUtilsTest, falseWhenNotPaused)
{
    ConfigurationData configurationData(
        { "https://sophosupdate.sophos.com/latest/warehouse" },
        Credentials{ "administrator", "password" },
        { "https://cache.sophos.com/latest/warehouse" },
        Proxy("noproxy:"));
    configurationData.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "" });
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::isEndpointPaused(configurationData),false);
}

TEST_F(SulDownloaderUtilsTest, trueWhenPausedOnPrimarySubscription)
{
    ConfigurationData configurationData(
        { "https://sophosupdate.sophos.com/latest/warehouse" },
        Credentials{ "administrator", "password" },
        { "https://cache.sophos.com/latest/warehouse" },
        Proxy("noproxy:"));
    configurationData.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "2023.1" });
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::isEndpointPaused(configurationData),true);
}

TEST_F(SulDownloaderUtilsTest, trueWhenPausedOnNonPrimarySubscription)
{
    ConfigurationData configurationData(
        { "https://sophosupdate.sophos.com/latest/warehouse" },
        Credentials{ "administrator", "password" },
        { "https://cache.sophos.com/latest/warehouse" },
        Proxy("noproxy:"));
    configurationData.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "" });
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::isEndpointPaused(configurationData),true);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToTrueNON_PAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedUpdate(true);

    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),true);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToFalseNON_PAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedUpdate(false);

    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenPauseIsEnabledNON_PAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedUpdate(true);
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenMarkerFileExistsNON_PAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedUpdate(true);
    EXPECT_CALL(*m_fileSystemMock, isFile(markerFile)).WillOnce(Return(true));
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToTruePAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedPausedUpdate(true);
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),true);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToFalsePAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedPausedUpdate(false);
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenPauseIsNotEnabledPAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedPausedUpdate(true);
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenMarkerFileExistsPAUSED_UPDATE)
{
    ConfigurationData configurationData(
        { "" });
    configurationData.setDoForcedPausedUpdate(true);
    configurationData.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    EXPECT_CALL(*m_fileSystemMock, isFile(pausedMarkerFile)).WillOnce(Return(true));
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(configurationData),false);
}