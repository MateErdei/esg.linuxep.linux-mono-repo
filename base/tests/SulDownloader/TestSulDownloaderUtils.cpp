// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/SulDownloader/SulDownloaderUtils.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Policy/UpdateSettings.h"
#include "Common/ProcessImpl/ProcessImpl.h"

#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include "tests/Common/Helpers/MockProcess.h"

#include <gtest/gtest.h>

using namespace Common::Policy;

class SulDownloaderUtilsTest : public ::testing::Test
{
    public:
        UpdateSettings getUpdateSettings(UpdateSettings::UpdateCacheHosts_t updateCache,
                                         UpdateSettings::url_list_t urls,
                                         Proxy proxy)
        {
            UpdateSettings updateSettings;
            updateSettings.setSophosLocationURLs(std::move(urls));
            updateSettings.setLocalUpdateCacheHosts(std::move(updateCache));
            updateSettings.setPolicyProxy(std::move(proxy));
            return updateSettings;
        }
        UpdateSettings getUpdateSettings(UpdateSettings::UpdateCacheHosts_t updateCache,
                                         Credentials,
                                         UpdateSettings::url_list_t urls,
                                         Proxy proxy)
        {
            UpdateSettings updateSettings;
            updateSettings.setSophosLocationURLs(std::move(urls));
            updateSettings.setLocalUpdateCacheHosts(std::move(updateCache));
            updateSettings.setPolicyProxy(std::move(proxy));
            return updateSettings;
        }
};

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
    auto updateSettings = getUpdateSettings({ "https://cache.sophos.com/latest/warehouse" },
                                            { "https://sophosupdate.sophos.com/latest/warehouse" },
                                             Proxy("noproxy:"));

    updateSettings.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "" });
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::isEndpointPaused(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTest, trueWhenPausedOnPrimarySubscription)
{
    UpdateSettings updateSettings = getUpdateSettings(
        { "https://sophosupdate.sophos.com/latest/warehouse" },
        Credentials{ "administrator", "password" },
        { "https://cache.sophos.com/latest/warehouse" },
        Proxy("noproxy:"));
    updateSettings.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "2023.1" });
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::isEndpointPaused(updateSettings),true);
}

TEST_F(SulDownloaderUtilsTest, trueWhenPausedOnNonPrimarySubscription)
{
    UpdateSettings updateSettings = getUpdateSettings(
        { "https://sophosupdate.sophos.com/latest/warehouse" },
        Credentials{ "administrator", "password" },
        { "https://cache.sophos.com/latest/warehouse" },
        Proxy("noproxy:"));
    updateSettings.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "" });
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });

    EXPECT_TRUE(SulDownloader::SulDownloaderUtils::isEndpointPaused(updateSettings));
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToTrueNON_PAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedUpdate(true);

    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),true);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToFalseNON_PAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedUpdate(false);

    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenPauseIsEnabledNON_PAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedUpdate(true);
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenMarkerFileExistsNON_PAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedUpdate(true);
    EXPECT_CALL(*m_fileSystemMock, isFile(markerFile)).WillOnce(Return(true));
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToTruePAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedPausedUpdate(true);
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),true);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnTrueWhenConfigSetToFalsePAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedPausedUpdate(false);
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenPauseIsNotEnabledPAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedPausedUpdate(true);
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTestFiles, checkIfWeShouldForceUpdatesReturnFalseWhenMarkerFileExistsPAUSED_UPDATE)
{
    UpdateSettings updateSettings;
    updateSettings.setDoForcedPausedUpdate(true);
    updateSettings.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "2023.1" } });
    EXPECT_CALL(*m_fileSystemMock, isFile(pausedMarkerFile)).WillOnce(Return(true));
    ASSERT_EQ(SulDownloader::SulDownloaderUtils::checkIfWeShouldForceUpdates(updateSettings),false);
}

TEST_F(SulDownloaderUtilsTestFiles, testComponentRunning)
{
    std::vector<std::string> lines = {"edr,ServerProtectionLinux-Plugin-EDR"};
    std::vector<std::string> failed = {};
    EXPECT_CALL(*m_fileSystemMock, isFile(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderInstalledTrackerFile())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readLines(_)).WillOnce(Return(lines));
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&]() {
            auto* mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> start_args = {"isrunning","edr"};
            EXPECT_CALL(*mockProcess, exec(HasSubstr("wdctl"), start_args)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);});
    ASSERT_EQ(failed,SulDownloader::SulDownloaderUtils::checkUpdatedComponentsAreRunning());
}

TEST_F(SulDownloaderUtilsTestFiles, testComponentNotRunning)
{
    std::vector<std::string> lines = {"edr,ServerProtectionLinux-Plugin-EDR"};
    std::vector<std::string> failed = {"ServerProtectionLinux-Plugin-EDR"};
    EXPECT_CALL(*m_fileSystemMock, isFile(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderInstalledTrackerFile())).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, readLines(_)).WillOnce(Return(lines));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&]() {
                                                                       auto* mockProcess = new StrictMock<MockProcess>();
                                                                       std::vector<std::string> start_args = {"isrunning","edr"};
                                                                       EXPECT_CALL(*mockProcess, exec(HasSubstr("wdctl"), start_args)).Times(1);
                                                                       EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                                                                       EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(4));
                                                                       return std::unique_ptr<Common::Process::IProcess>(mockProcess);});
    ASSERT_EQ(failed, SulDownloader::SulDownloaderUtils::checkUpdatedComponentsAreRunning());
}