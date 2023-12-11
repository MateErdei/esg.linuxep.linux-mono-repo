// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "Common/WatchdogConstants/WatchdogConstants.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "watchdog/watchdogimpl/Watchdog.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace
{
    class TestWatchdog : public LogOffInitializedTests
    {
        Tests::ScopedReplaceFileSystem m_fileSystemReplacer;
        Tests::ScopedReplaceFilePermissions m_filePermissionsReplacer;

    protected:
        MockFilePermissions* m_mockFilePermissionsPtr;
        MockFileSystem* m_mockFileSystemPtr;
        std::string m_actualUserGroupIdConfigPath;
        std::string m_requestedUserGroupIdConfigPath;
        std::string m_installOptionsPath;

    public:
        TestWatchdog()
        {
            m_mockFileSystemPtr = new StrictMock<MockFileSystem>();
            m_mockFilePermissionsPtr = new NiceMock<MockFilePermissions>();
            m_fileSystemReplacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystemPtr));
            m_filePermissionsReplacer.replace(std::unique_ptr<Common::FileSystem::IFilePermissions>(m_mockFilePermissionsPtr));

            m_actualUserGroupIdConfigPath = Common::ApplicationConfiguration::applicationPathManager().getActualUserGroupIdConfigPath();
            m_requestedUserGroupIdConfigPath = Common::ApplicationConfiguration::applicationPathManager().getRequestedUserGroupIdConfigPath();

            EXPECT_CALL(*m_mockFileSystemPtr, isDirectory(HasSubstr("base/telemetry/cache"))).WillRepeatedly(Return(false));
            EXPECT_CALL(*m_mockFileSystemPtr, isFile(HasSubstr("base/telemetry/cache"))).WillRepeatedly(Return(false));
            std::string pluginname = "plugins/" + Common::WatchdogConstants::WatchdogServiceLineName() + ".ipc";
            Common::ApplicationConfiguration::applicationConfiguration().setData(pluginname, "inproc://watchdogservice.ipc");
        }
        ~TestWatchdog()
        {
            Tests::restoreFilePermissions();
        }
    };

    class TestableWatchdog : public watchdog::watchdogimpl::Watchdog
    {
    public:
        explicit TestableWatchdog(Common::ZMQWrapperApi::IContextSharedPtr context) :
            watchdog::watchdogimpl::Watchdog(std::move(context))
        {
        }

        explicit TestableWatchdog() = default;

        void callSetupIpc() { setupSocket(); }

        void callHandleSocketRequest() { handleSocketRequest(); };

        void callWriteExecutableUserAndGroupToActualUserGroupIdConfig() { writeExecutableUserAndGroupToActualUserGroupIdConfig(); };

        /**
         * Give access to the context so that we can share connections
         * @return
         */
        Common::ZMQWrapperApi::IContext& context() { return *m_context; }

        void addPlugin(const std::string& pluginName)
        {
            Common::PluginRegistryImpl::PluginInfo info;
            info.setPluginName(pluginName);
            addProcessToMonitor(std::unique_ptr<watchdog::watchdogimpl::PluginProxy>(
                new watchdog::watchdogimpl::PluginProxy(std::move(info))));
        }
    };
} // namespace

std::string createPluginRegistryJson(const std::string& pluginName, const std::string& userAndGroup)
{
    nlohmann::json pluginJson;
    pluginJson["policyAppIds"] = "";
    pluginJson["actionAppIds"] = "";
    pluginJson["statusAppIds"] = "";
    pluginJson["pluginName"] = pluginName;
    pluginJson["executableFullPath"] = "";
    pluginJson["executableArguments"] = {};
    pluginJson["environmentVariables"] = {};
    pluginJson["threatServiceHealth"] = true;
    pluginJson["serviceHealth"] = true;
    pluginJson["displayPluginName"] = "Display Name";
    pluginJson["executableUserAndGroup"] = userAndGroup;
    pluginJson["secondsToShutDown"] = 10;

    return pluginJson.dump();
}

TEST_F(TestWatchdog, Construction)
{
    EXPECT_NO_THROW(watchdog::watchdogimpl::Watchdog watchdog);
    EXPECT_NO_THROW(TestableWatchdog watchdog2);
}

TEST_F(TestWatchdog, ipcStartup)
{
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", "inproc://ipcStartup");
    TestableWatchdog watchdog;
    EXPECT_NO_THROW(watchdog.callSetupIpc());
}

TEST_F(TestWatchdog, stopPluginViaIPC_missing_plugin)
{
    const std::string IPC_ADDRESS = "inproc://stopPluginViaIPC";
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", IPC_ADDRESS);
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);
    watchdog.callSetupIpc();
    Common::ZeroMQWrapper::ISocketRequesterPtr requester = context->getRequester();
    requester->connect(IPC_ADDRESS);
    requester->write({ "STOP", "mcsrouter" });

    watchdog.callHandleSocketRequest();

    Common::ZeroMQWrapper::IReadable::data_t result = requester->read();
    EXPECT_EQ(result.at(0), "Error: Plugin not found");
}

TEST_F(TestWatchdog, stopPluginViaIPC_test_plugin)
{
    const std::string IPC_ADDRESS = "inproc://stopPluginViaIPC_test_plugin";
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", IPC_ADDRESS);
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);
    watchdog.callSetupIpc();

    watchdog.addPlugin("mcsrouter");

    Common::ZeroMQWrapper::ISocketRequesterPtr requester = context->getRequester();
    requester->connect(IPC_ADDRESS);
    requester->write({ "STOP", "mcsrouter" });

    watchdog.callHandleSocketRequest();

    Common::ZeroMQWrapper::IReadable::data_t result = requester->read();
    EXPECT_EQ(result.at(0), Common::WatchdogConstants::Response::watchdogReturnsOk);
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigWritesConfigFile)
{
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);

    std::vector<std::string> files{ "/tmp/plugins/PluginName.json" };
    EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[0]))
        .WillOnce(Return(createPluginRegistryJson("PluginName", "user:group")));

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_actualUserGroupIdConfigPath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user")).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("group")).WillRepeatedly(Return(2));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-ipc")).WillOnce(Return(0));

    // Match indentation of the file being written by the product, we want it pretty printed because it's user facing
    nlohmann::json expectedJson =
        nlohmann::json::parse(R"({"groups":{"group":2,"sophos-spl-ipc":0},"users":{"user":1}})");
    std::string expectedJsonString = expectedJson.dump(4);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_actualUserGroupIdConfigPath, expectedJsonString)).Times(1);

    EXPECT_NO_THROW(watchdog.callWriteExecutableUserAndGroupToActualUserGroupIdConfig());
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigHandlesDuplicateUsersAndGroups)
{
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);

    std::vector<std::string> files{ "/tmp/plugins/PluginName1.json", "/tmp/plugins/PluginName2.json" };
    EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[0]))
        .WillOnce(Return(createPluginRegistryJson("PluginName1", "user:group")));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[1]))
        .WillOnce(Return(createPluginRegistryJson("PluginName2", "user:group")));

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_actualUserGroupIdConfigPath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user")).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("group")).WillRepeatedly(Return(2));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-ipc")).WillOnce(Return(0));

    // Match indentation of the file being written by the product, we want it pretty printed because it's user facing
    nlohmann::json expectedJson =
        nlohmann::json::parse(R"({"groups":{"group":2,"sophos-spl-ipc":0},"users":{"user":1}})");
    std::string expectedJsonString = expectedJson.dump(4);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_actualUserGroupIdConfigPath, expectedJsonString)).Times(1);
    EXPECT_NO_THROW(watchdog.callWriteExecutableUserAndGroupToActualUserGroupIdConfig());
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigOverwritesExistingConfigFile)
{
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);

    std::vector<std::string> files{ "/tmp/plugins/PluginName.json" };
    EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[0]))
        .WillOnce(Return(createPluginRegistryJson("PluginName", "user:group")));

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_actualUserGroupIdConfigPath)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user")).WillOnce(Return(999));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("group")).WillRepeatedly(Return(966));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-ipc")).WillRepeatedly(Return(0));

    // Match indentation of the file being written by the product, we want it pretty printed because it's user facing
    nlohmann::json expectedJson =
        nlohmann::json::parse(R"({"groups":{"group":966,"sophos-spl-ipc":0},"users":{"user":999}})");
    std::string expectedJsonString = expectedJson.dump(4);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_actualUserGroupIdConfigPath, expectedJsonString)).Times(1);
    EXPECT_NO_THROW(watchdog.callWriteExecutableUserAndGroupToActualUserGroupIdConfig());
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigHandlesMultipleGroupsAndUsers)
{
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);

    std::vector<std::string> files{ "/tmp/plugins/PluginName1.json",
                                    "/tmp/plugins/PluginName2.json",
                                    "/tmp/plugins/PluginName3.json",
                                    "/tmp/plugins/PluginName4.json" };
    EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[0]))
        .WillOnce(Return(createPluginRegistryJson("PluginName1", "user1:group1")));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[1]))
        .WillOnce(Return(createPluginRegistryJson("PluginName2", "user2")));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[2]))
        .WillOnce(Return(createPluginRegistryJson("PluginName3", "user3:group3")));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[3]))
        .WillOnce(Return(createPluginRegistryJson("PluginName4", "user4")));

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_actualUserGroupIdConfigPath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user1")).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user2")).WillOnce(Return(2));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user3")).WillOnce(Return(3));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user4")).WillOnce(Return(4));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("group1")).WillRepeatedly(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("group3")).WillRepeatedly(Return(3));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-ipc")).WillRepeatedly(Return(0));

    // Match indentation of the file being written by the product, we want it pretty printed because it's user facing
    nlohmann::json expectedJson = nlohmann::json::parse(
        R"({"groups":{"group1":1,"group3":3,"sophos-spl-ipc":0},"users":{"user1":1,"user2":2,"user3":3,"user4":4}})");
    std::string expectedJsonString = expectedJson.dump(4);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_actualUserGroupIdConfigPath, expectedJsonString)).Times(1);
    EXPECT_NO_THROW(watchdog.callWriteExecutableUserAndGroupToActualUserGroupIdConfig());
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigIgnoresRootUser)
{
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);

    std::vector<std::string> files{ "/tmp/plugins/PluginName.json" };
    EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[0]))
        .WillOnce(Return(createPluginRegistryJson("PluginName", "root")));

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_actualUserGroupIdConfigPath)).WillOnce(Return(false));

    // Match indentation of the file being written by the product, we want it pretty printed because it's user facing
    nlohmann::json expectedJson = nlohmann::json::parse(R"({"groups":{"sophos-spl-ipc":0}})");
    std::string expectedJsonString = expectedJson.dump(4);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_actualUserGroupIdConfigPath, expectedJsonString)).Times(1);
    EXPECT_NO_THROW(watchdog.callWriteExecutableUserAndGroupToActualUserGroupIdConfig());
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigHandlesMalformedGroupsAndUsers)
{
    Common::ZMQWrapperApi::IContextSharedPtr context(Common::ZMQWrapperApi::createContext());
    TestableWatchdog watchdog(context);

    std::vector<std::string> files{ "/tmp/plugins/PluginName1.json",
                                    "/tmp/plugins/PluginName2.json",
                                    "/tmp/plugins/PluginName3.json" };
    EXPECT_CALL(*m_mockFileSystemPtr, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[0]))
        .WillOnce(Return(createPluginRegistryJson("PluginName1", "user1group1")));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[1]))
        .WillOnce(Return(createPluginRegistryJson("PluginName2", "user2:")));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(files[2]))
        .WillOnce(Return(createPluginRegistryJson("PluginName3", "user3::")));

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_actualUserGroupIdConfigPath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("user1group1"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST")));

    EXPECT_NO_THROW(watchdog.callWriteExecutableUserAndGroupToActualUserGroupIdConfig());
}

class TestC
{
public:
    explicit TestC(int pos) : m_running(false), m_pos(pos) {}

    TestC(TestC&& other) noexcept : m_running(false)
    {
        std::swap(m_running, other.m_running);
        m_pos = 5;
    }

    ~TestC()
    {
        std::cerr << "Deleting " << m_pos << " running=" << m_running << std::endl;
        EXPECT_FALSE(m_running);
    }

    TestC& operator=(TestC&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        std::swap(m_running, other.m_running);
        m_pos = other.m_pos;
        return *this;
    }

    bool m_running;
    int m_pos;
};

TEST(TestVectorEraseMoves, TestBoolCopiedCorrectly)
{
    std::vector<TestC> values;
    values.emplace_back(0);
    values.emplace_back(1);
    values.emplace_back(2);

    values[0].m_running = true;
    values[1].m_running = true;
    values[2].m_running = true;

    values[0].m_running = false;
    values.erase(values.begin());

    values[0].m_running = false;
    values[1].m_running = false;
    values.clear();
}

TEST(TestList, TestBoolCorrect)
{
    std::list<TestC> values;
    values.emplace_back(0);
    values.emplace_back(1);
    values.emplace_back(2);

    for (auto& v : values)
    {
        v.m_running = true;
    }

    values.begin()->m_running = false;
    values.erase(values.begin());

    for (auto& v : values)
    {
        v.m_running = false;
    }
    values.clear();
}