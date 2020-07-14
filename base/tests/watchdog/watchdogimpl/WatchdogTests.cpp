/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/PluginRegistryImpl/PluginRegistryException.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <watchdog/watchdogimpl/Watchdog.h>
#include <watchdog/watchdogimpl/watchdog_main.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

namespace
{
    class TestWatchdog : public watchdog::watchdogimpl::Watchdog
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;

    public:
        TestWatchdog() {}
        watchdog::watchdogimpl::PluginInfoVector call_read_plugin_configs() { return readPluginConfigs(); }
    };

    using ::testing::NiceMock;

    class WatchdogTests  : public LogOffInitializedTests
    {
    public:
        void SetUp() override
        {
            MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager& mock(*mockAppManager);
            ON_CALL(mock, sophosInstall()).WillByDefault(Return("/tmp/sophos"));
            ON_CALL(mock, getPluginRegistryPath()).WillByDefault(Return("/tmp/plugins"));
            ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://watchdogservice.ipc"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

            m_mockFileSystemPtr = new StrictMock<MockFileSystem>();
            m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystemPtr));
            EXPECT_CALL(*m_mockFileSystemPtr, isFile(HasSubstr("base/telemetry/cache"))).WillRepeatedly(Return(false));
        }

        void TearDown() override { Common::ApplicationConfiguration::restoreApplicationPathManager(); }
        IgnoreFilePermissions ignoreFilePermissions;
        MockFileSystem* m_mockFileSystemPtr;
        Tests::ScopedReplaceFileSystem m_replacer; 

    };

    std::string createJsonString(const std::string& oldPartString, const std::string& newPartString)
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

        if (!oldPartString.empty())
        {
            size_t pos = jsonString.find(oldPartString);

            EXPECT_NE(pos, std::string::npos);

            jsonString.replace(pos, oldPartString.size(), newPartString);
        }

        return jsonString;
    }

    TEST_F(WatchdogTests, WatchdogCanReadSinglePluginConfig) // NOLINT
    {
        std::vector<std::string> files;
        std::string filename("/tmp/plugins/PluginName.json");
        files.push_back(filename);
        std::string fileContent = createJsonString("", "");

        EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*m_mockFileSystemPtr, readFile(filename)).WillOnce(Return(fileContent));

        TestWatchdog watchdog;
        watchdog::watchdogimpl::PluginInfoVector plugins;

        EXPECT_NO_THROW(plugins = watchdog.call_read_plugin_configs()); // NOLINT

        EXPECT_EQ(plugins.size(), 1);

    }

    TEST_F(WatchdogTests, WatchdogShouldNoFailIfNoValidPluginConfigs) // NOLINT
    {
        std::vector<std::string> files;
        std::string filename("/tmp/plugins/invalid.json");
        files.push_back(filename);
        EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*m_mockFileSystemPtr, readFile(filename)).WillOnce(Return("invalid json"));

        TestWatchdog watchdog;

        EXPECT_NO_THROW(watchdog.call_read_plugin_configs()); // NOLINT
    }

    TEST_F(WatchdogTests, WatchdogSucceedsIfAnyValidPluginConfigs) // NOLINT
    {
        std::vector<std::string> files;
        std::string filename1("/tmp/plugins/valid.json");
        std::string filename2("/tmp/plugins/invalid.json");
        files.push_back(filename1);
        files.push_back(filename2);
        std::string fileContent = createJsonString("PluginName", "valid");

        EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*m_mockFileSystemPtr, readFile(filename1)).WillOnce(Return(fileContent));
        EXPECT_CALL(*m_mockFileSystemPtr, readFile(filename2)).WillOnce(Return("invalid json"));

        TestWatchdog watchdog;

        watchdog::watchdogimpl::PluginInfoVector plugins;

        EXPECT_NO_THROW(plugins = watchdog.call_read_plugin_configs()); // NOLINT

        EXPECT_EQ(plugins.size(), 1);

    }

    TEST_F(WatchdogTests, WatchdogSucceedsWhenItLoadsTwoPluginConfigs) // NOLINT
    {
        std::vector<std::string> files;
        std::string filename1("/tmp/plugins/valid1.json");
        std::string filename2("/tmp/plugins/valid2.json");
        files.push_back(filename1);
        files.push_back(filename2);
        std::string fileContent1 = createJsonString("PluginName", "valid1");
        std::string fileContent2 = createJsonString("PluginName", "valid2");

        EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*m_mockFileSystemPtr, readFile(filename1)).WillOnce(Return(fileContent1));
        EXPECT_CALL(*m_mockFileSystemPtr, readFile(filename2)).WillOnce(Return(fileContent2));

        TestWatchdog watchdog;

        watchdog::watchdogimpl::PluginInfoVector plugins;

        EXPECT_NO_THROW(plugins = watchdog.call_read_plugin_configs()); // NOLINT

        EXPECT_EQ(plugins.size(), 2);

    }
} // namespace
