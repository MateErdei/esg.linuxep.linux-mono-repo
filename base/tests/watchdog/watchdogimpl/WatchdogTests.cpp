/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <watchdog/watchdogimpl/watchdog_main.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/PluginRegistryImpl/PluginRegistryException.h>

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace
{
    class TestWatchdog
        : public watchdog::watchdogimpl::watchdog_main
    {
    public:
        watchdog::watchdogimpl::PluginInfoVector call_read_plugin_configs()
        {
            return read_plugin_configs();
        }
    };

    using ::testing::NiceMock;

    class WatchdogTests : public ::testing::Test
    {

    public:

        void SetUp() override
        {
            MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager &mock(*mockAppManager);
            ON_CALL(mock, sophosInstall()).WillByDefault(Return("/tmp/sophos"));
            ON_CALL(mock, getPluginRegistryPath()).WillByDefault(Return("/tmp/plugins"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                    std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));
        }

        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
        }

    };

    std::string createJsonString(const std::string & oldPartString, const std::string & newPartString)
    {
        std::string jsonString = R"({
                                    "policyAppIds": [
                                    "app1"
                                     ],
                                     "statusAppIds": [
                                      "app2"
                                     ],
                                     "pluginName": "PluginName",
                                     "xmlTranslatorPath": "plugin/lib/xmlTranslator.so",
                                     "executableFullPath": "plugin/bin/executable",
                                     "executableArguments": [
                                      "arg1"
                                     ],
                                     "environmentVariables": [
                                      {
                                       "name": "hello",
                                       "value": "world"
                                      }
                                     ]
                                    })";

        if(!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_TRUE(pos != std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);

        }

        return jsonString;
    }


    TEST_F(WatchdogTests, WatchdogCanReadSinglePluginConfig) //NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        std::vector<std::string> files;
        std::string filename("/tmp/plugins/valid.json");
        files.push_back(filename);
        std::string fileContent = createJsonString("", "");

        EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*mockFileSystem, readFile(filename)).WillOnce(Return(fileContent));

        TestWatchdog watchdog;
        watchdog::watchdogimpl::PluginInfoVector plugins;

        EXPECT_NO_THROW(plugins = watchdog.call_read_plugin_configs());

        EXPECT_EQ(plugins.size(),1);

        Common::FileSystem::restoreFileSystem();
    }

    TEST_F(WatchdogTests, WatchdogFailsIfNoValidPluginConfigs) //NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        std::vector<std::string> files;
        std::string filename("/tmp/plugins/invalid.json");
        files.push_back(filename);
        EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*mockFileSystem, readFile(filename)).WillOnce(Return("invalid json"));

        TestWatchdog watchdog;

        EXPECT_THROW(watchdog.call_read_plugin_configs(), Common::PluginRegistryImpl::PluginRegistryException);

        Common::FileSystem::restoreFileSystem();
    }

    TEST_F(WatchdogTests, WatchdogSucceedsIfAnyValidPluginConfigs) //NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        std::vector<std::string> files;
        std::string filename1("/tmp/plugins/valid.json");
        std::string filename2("/tmp/plugins/invalid.json");
        files.push_back(filename1);
        files.push_back(filename2);
        std::string fileContent = createJsonString("", "");

        EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Return(fileContent));
        EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Return("invalid json"));

        TestWatchdog watchdog;

        watchdog::watchdogimpl::PluginInfoVector plugins;

        EXPECT_NO_THROW(plugins = watchdog.call_read_plugin_configs());

        EXPECT_EQ(plugins.size(),1);

        Common::FileSystem::restoreFileSystem();
    }


    TEST_F(WatchdogTests, WatchdogSucceedsWhenItLoadsTwoPluginConfigs) // NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        std::vector<std::string> files;
        std::string filename1("/tmp/plugins/valid1.json");
        std::string filename2("/tmp/plugins/valid2.json");
        files.push_back(filename1);
        files.push_back(filename2);
        std::string fileContent = createJsonString("", "");

        EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*mockFileSystem, readFile(filename1)).WillOnce(Return(fileContent));
        EXPECT_CALL(*mockFileSystem, readFile(filename2)).WillOnce(Return(fileContent));

        TestWatchdog watchdog;

        watchdog::watchdogimpl::PluginInfoVector plugins;

        EXPECT_NO_THROW(plugins = watchdog.call_read_plugin_configs());

        EXPECT_EQ(plugins.size(),2);

        Common::FileSystem::restoreFileSystem();
    }
}
