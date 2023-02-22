/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>

#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <watchdog/watchdogimpl/Watchdog.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    class TestWatchdog : public LogOffInitializedTests
    {
        Tests::ScopedReplaceFileSystem m_fileSystemReplacer;
        Tests::ScopedReplaceFilePermissions m_filePermissionsReplacer;

    protected:
        MockFilePermissions* m_mockFilePermissionsPtr;
        MockFileSystem* m_mockFileSystemPtr;
        std::string m_watchdogConfigPath;
    public:
        TestWatchdog()
        {
            m_mockFileSystemPtr = new StrictMock<MockFileSystem>();
            m_mockFilePermissionsPtr = new NiceMock<MockFilePermissions>();
            m_fileSystemReplacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystemPtr));
            m_filePermissionsReplacer.replace(std::unique_ptr<Common::FileSystem::IFilePermissions>(m_mockFilePermissionsPtr));

            m_watchdogConfigPath = Common::ApplicationConfiguration::applicationPathManager().getWatchdogConfigPath();

            EXPECT_CALL(*m_mockFileSystemPtr, isDirectory(HasSubstr("base/telemetry/cache"))).WillRepeatedly(Return(false));
            EXPECT_CALL(*m_mockFileSystemPtr, isFile(HasSubstr("base/telemetry/cache"))).WillRepeatedly(Return(false));
            std::string pluginname =
                    "plugins/" + watchdog::watchdogimpl::WatchdogServiceLine::WatchdogServiceLineName() + ".ipc";
            Common::ApplicationConfiguration::applicationConfiguration().setData(pluginname,
                    "inproc://watchdogservice.ipc");

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

TEST_F(TestWatchdog, Construction) // NOLINT
{
    EXPECT_NO_THROW(watchdog::watchdogimpl::Watchdog watchdog); // NOLINT
    EXPECT_NO_THROW(TestableWatchdog watchdog2);                // NOLINT
}

TEST_F(TestWatchdog, ipcStartup) // NOLINT
{
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc", "inproc://ipcStartup");
    TestableWatchdog watchdog;
    EXPECT_NO_THROW(watchdog.callSetupIpc()); // NOLINT
}

TEST_F(TestWatchdog, stopPluginViaIPC_missing_plugin) // NOLINT
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

TEST_F(TestWatchdog, stopPluginViaIPC_test_plugin) // NOLINT
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
    EXPECT_EQ(result.at(0), watchdog::watchdogimpl::watchdogReturnsOk);
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigWritesConfigFile)
{
    std::string userAndGroup = "sophos-spl-user:sophos-spl-group";
    std::string expectedWatchdogConfig = R"({"groups":{"sophos-spl-group":2},"users":{"sophos-spl-user":1}})";

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_watchdogConfigPath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("sophos-spl-user")).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group")).WillRepeatedly(Return(2));

    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig)).Times(1);

    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup));
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigUpdatesExistingConfigFile)
{
    std::string userAndGroup1 = "sophos-spl-user1:sophos-spl-group1";
    std::string userAndGroup2 = "sophos-spl-user2:sophos-spl-group2";

    std::string expectedWatchdogConfig = R"({"groups":{"sophos-spl-group1":1},"users":{"sophos-spl-user1":1}})";
    std::string expectedWatchdogConfig2 = R"({"groups":{"sophos-spl-group1":1,"sophos-spl-group2":2},"users":{"sophos-spl-user1":1,"sophos-spl-user2":2}})";

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_watchdogConfigPath)).Times(2)
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(m_watchdogConfigPath)).WillOnce(Return(expectedWatchdogConfig));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("sophos-spl-user1")).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("sophos-spl-user2")).WillOnce(Return(2));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group1")).WillRepeatedly(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group2")).WillRepeatedly(Return(2));

    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig)).Times(1);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig2)).Times(1);

    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup1));
    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup2));
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigHandlesMultipleGroupsAndUsers)
{
    std::string userAndGroup1 = "sophos-spl-user1:sophos-spl-group1";
    std::string user2 = "sophos-spl-user2";
    std::string userAndGroup3 = "sophos-spl-user3:sophos-spl-group3";
    std::string user4 = "sophos-spl-user4";

    std::string expectedWatchdogConfig = R"({"groups":{"sophos-spl-group1":1},"users":{"sophos-spl-user1":1}})";
    std::string expectedWatchdogConfig2 = R"({"groups":{"sophos-spl-group1":1},"users":{"sophos-spl-user1":1,"sophos-spl-user2":2}})";
    std::string expectedWatchdogConfig3 = R"({"groups":{"sophos-spl-group1":1,"sophos-spl-group3":3},"users":{"sophos-spl-user1":1,"sophos-spl-user2":2,"sophos-spl-user3":3}})";
    std::string expectedWatchdogConfig4 = R"({"groups":{"sophos-spl-group1":1,"sophos-spl-group3":3},"users":{"sophos-spl-user1":1,"sophos-spl-user2":2,"sophos-spl-user3":3,"sophos-spl-user4":4}})";


    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_watchdogConfigPath)).Times(4)
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(m_watchdogConfigPath)).Times(3)
        .WillOnce(Return(expectedWatchdogConfig))
        .WillOnce(Return(expectedWatchdogConfig2))
        .WillOnce(Return(expectedWatchdogConfig3));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("sophos-spl-user1")).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId(user2)).WillOnce(Return(2));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId("sophos-spl-user3")).WillOnce(Return(3));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId(user4)).WillOnce(Return(4));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group1")).WillRepeatedly(Return(1));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group2")).WillRepeatedly(Return(2));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group3")).WillRepeatedly(Return(3));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getGroupId("sophos-spl-group4")).WillRepeatedly(Return(4));

    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig)).Times(1);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig2)).Times(1);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig3)).Times(1);
    EXPECT_CALL(*m_mockFileSystemPtr, writeFile(m_watchdogConfigPath, expectedWatchdogConfig4)).Times(1);

    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup1));
    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(user2));
    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup3));
    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(user4));
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigIgnoresNonSPLUsersAndGroups)
{
    std::string userAndGroup = "user:group";
    std::string user = "root";

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_watchdogConfigPath)).Times(2).WillRepeatedly(Return(false));

    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup));
    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(user));
}

TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigHandlesMalformedGroupsAndUsers)
{
    std::string userAndGroup = "sophos-spl-user1sophos-spl-group1";
    std::string user = "sophos-spl-user2:";

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_watchdogConfigPath)).Times(2).WillRepeatedly(Return(false));

    EXPECT_CALL(*m_mockFilePermissionsPtr, getUserId(userAndGroup)).WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST")));

    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup));
    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(user));
}


TEST_F(TestWatchdog, writeExecutableUserAndGroupToWatchdogConfigDoesNotThrowWithMalformedExistingConfigs)
{
    std::string userAndGroup = "sophos-spl-user:sophos-spl-group";
    std::string malformedWatchdogConfig = R"({"groups":{"g":{"sophos-spl-user":1}})";

    EXPECT_CALL(*m_mockFileSystemPtr, isFile(m_watchdogConfigPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystemPtr, readFile(m_watchdogConfigPath)).WillOnce(Return(malformedWatchdogConfig));

    EXPECT_NO_THROW(TestableWatchdog::writeExecutableUserAndGroupToWatchdogConfig(userAndGroup));
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

TEST(TestVectorEraseMoves, TestBoolCopiedCorrectly) // NOLINT
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

TEST(TestList, TestBoolCorrect) // NOLINT
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