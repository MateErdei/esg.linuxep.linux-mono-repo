/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Watchdog.h"
#include "MockedApplicationPathManager.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/PluginRegistryImpl/PluginRegistryException.h"
#include "../../tests/Common/FileSystemImpl/MockFileSystem.h"


namespace
{
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


    TEST_F(WatchdogTests, WatchdogCanReadSinglePluginConfig)
    {
        auto mockFileSystem = new MockFileSystem();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        std::vector<std::string> files;
        std::string filename("/tmp/plugins/valid.json");
        files.push_back(filename);
        std::string fileContent = createJsonString("", "");

        EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*mockFileSystem, readFile(filename)).WillOnce(Return(fileContent));

        EXPECT_NO_THROW(Watchdog::read_plugin_configs());

        Common::FileSystem::restoreFileSystem();
    }

    TEST_F(WatchdogTests, WatchdogFailsIfNoValidPluginConfigs)
    {
        auto mockFileSystem = new MockFileSystem();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
        Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

        std::vector<std::string> files;
        std::string filename("/tmp/plugins/invalid.json");
        files.push_back(filename);
        EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
        EXPECT_CALL(*mockFileSystem, readFile(filename)).WillOnce(Return("invalid json"));

        EXPECT_THROW(Watchdog::read_plugin_configs(), Common::PluginRegistryImpl::PluginRegistryException);

        Common::FileSystem::restoreFileSystem();
    }

    TEST_F(WatchdogTests, WatchdogSucceedsIfAnyValidPluginConfigs)
    {
        auto mockFileSystem = new MockFileSystem();
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
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

        EXPECT_NO_THROW(Watchdog::read_plugin_configs());

        Common::FileSystem::restoreFileSystem();
    }

}
