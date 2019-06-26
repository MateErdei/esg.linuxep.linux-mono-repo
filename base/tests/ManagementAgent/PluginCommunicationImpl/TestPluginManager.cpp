/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <ManagementAgent/PluginCommunication/IPluginCommunicationException.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginProxy.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/PluginApiImpl/MockedPluginApiCallback.h>

#include <thread>

using ManagementAgent::PluginCommunicationImpl::PluginProxy;

class TestPluginManager : public ::testing::Test
{
public:
    TestPluginManager()
    {
        m_pluginOneName = "plugin_one";
        m_pluginTwoName = "plugin_two";
        MockedApplicationPathManager* mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
        Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));
        ON_CALL(*mockApplicationPathManager, getManagementAgentSocketAddress())
            .WillByDefault(Return("inproc:///tmp/management.ipc"));
        ON_CALL(*mockApplicationPathManager, getPluginSocketAddress(m_pluginOneName))
            .WillByDefault(Return("inproc:///tmp/plugin_one"));
        ON_CALL(*mockApplicationPathManager, getPluginSocketAddress(m_pluginTwoName))
            .WillByDefault(Return("inproc:///tmp/plugin_two"));
        ON_CALL(*mockApplicationPathManager, getPublisherDataChannelAddress())
            .WillByDefault(Return("inproc:///tmp/pubchannel.ipc"));
        ON_CALL(*mockApplicationPathManager, getSubscriberDataChannelAddress())
            .WillByDefault(Return("inproc:///tmp/subchannel.ipc"));
        m_registryPath = "/registry";
        ON_CALL(*mockApplicationPathManager, getPluginRegistryPath()).WillByDefault(Return(m_registryPath));

        m_pluginManagerPtr.reset(new ManagementAgent::PluginCommunicationImpl::PluginManager(Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress()));

        m_mockedPluginApiCallback = std::make_shared<StrictMock<MockedPluginApiCallback>>();
        m_mgmtCommon.reset(new Common::PluginApiImpl::PluginResourceManagement(m_pluginManagerPtr->getSocketContext()));
        setupFileSystemAndGetMock();
        m_pluginApi = m_mgmtCommon->createPluginAPI(m_pluginOneName, m_mockedPluginApiCallback);
    }

    MockFileSystem& setupFileSystemAndGetMock()
    {
        std::string plugin_one_settings = R"sophos({
"policyAppIds": ["plugin_one"],
"statusAppIds": ["plugin_one"],
"pluginName": "plugin_one"})sophos";
        std::string plugin_two_settings = R"sophos({
"policyAppIds": ["plugin_two"],
"statusAppIds": ["plugin_two"],
"pluginName": "plugin_two"})sophos";

        auto filesystemMock = new NiceMock<MockFileSystem>();
        ON_CALL(*filesystemMock, listFiles(m_registryPath))
            .WillByDefault(
                Return(std::vector<std::string>{ { "/registry/plugin_one.json" }, { "/registry/plugin_two.json" } }));

        ON_CALL(*filesystemMock, isFile("/registry/plugin_one.json")).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isFile("/registry/plugin_two.json")).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, readFile("/registry/plugin_one.json")).WillByDefault(Return(plugin_one_settings));
        ON_CALL(*filesystemMock, readFile("/registry/plugin_two.json")).WillByDefault(Return(plugin_two_settings));

        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

        EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
        EXPECT_CALL(*mockFilePermissions, chown(_, _, _)).WillRepeatedly(Return());
        /*EXPECT_CALL(*filesystemMock, isDirectory("/installroot")).WillOnce(Return(true));
        EXPECT_CALL(*filesystemMock,
        isDirectory("/installroot/base/update/cache/primarywarehouse")).WillOnce(Return(true));
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot/base/update/cache/primary")).WillOnce(Return(true));
        EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, join(_,_)).WillRepeatedly(Invoke([](const std::string& a, const
        std::string&b){return a + "/" + b; }));*/
        auto pointer = filesystemMock;
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }

    ~TestPluginManager() override = default;

    std::string m_pluginOneName;
    std::string m_pluginTwoName;
    std::string m_pluginThreeName;
    std::string m_registryPath;
    std::unique_ptr<Common::PluginApiImpl::PluginResourceManagement> m_mgmtCommon;
    std::shared_ptr<MockedPluginApiCallback> m_mockedPluginApiCallback;
    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager> m_pluginManagerPtr;
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_pluginApi;
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_pluginApiTwo;

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestPluginManager, TestApplyPolicyOnRegisteredPlugin) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicy")).Times(1);
    std::thread applyPolicy(
        [this]() { EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginOneName, "testpolicy"), 1); });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyNotSentToRegisteredPluginWithWrongAppId) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicy")).Times(0);
    std::thread applyPolicy([this]() { EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicy"), 0); });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestAppIdCanBeChangedForRegisteredPluginForPolicy) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicynotsent")).Times(0);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicysent")).Times(1);
    std::thread applyPolicy([this]() {
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicynotsent"), 0);
        std::vector<std::string> appIds;
        appIds.emplace_back("wrongappid");
        m_pluginManagerPtr->registerAndSetAppIds(m_pluginOneName, appIds, appIds);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicysent"), 1);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyOnTwoRegisteredPlugins) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicyone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicytwo")).Times(1);
    std::thread applyPolicy([this]() {
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginOneName, "testpolicyone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginTwoName, "testpolicytwo"), 1);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyOnFailedPluginLeavesItInRegisteredPluginList) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicyone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicytwo")).Times(0);
    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, isFile("/registry/plugin_two.json"))
        .WillOnce(Return(true))  // Registeer
        .WillOnce(Return(true)); // Check it is still in Register
    std::thread applyPolicy([this]() {
        m_pluginManagerPtr->setDefaultConnectTimeout(10);
        m_pluginManagerPtr->setDefaultTimeout(10);
        // Register plugin with management and add to proxy list
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
        // Shutdown plugin - management agent doesn't know it has gone away until it attempts to communicate with it
        m_pluginApiTwo.reset();
        std::vector<std::string> pluginsBeforeRemoval = { m_pluginOneName, m_pluginTwoName };
        std::vector<std::string> pluginsAfterRemoval = { m_pluginOneName, m_pluginTwoName };
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginOneName, "testpolicyone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsBeforeRemoval);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginTwoName, "testpolicytwo"), 0);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsAfterRemoval);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyOnPluginNoLongerInstalledRemovesItFromRegisteredPluginList) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicyone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicytwo")).Times(0);
    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, isFile("/registry/plugin_two.json"))
        .WillOnce(Return(true))   // Registeer
        .WillOnce(Return(false)); // Check it is still in Register
    std::thread applyPolicy([this]() {
        m_pluginManagerPtr->setDefaultConnectTimeout(10);
        m_pluginManagerPtr->setDefaultTimeout(10);
        // Register plugin with management and add to proxy list
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
        // Shutdown plugin - management agent doesn't know it has gone away until it attempts to communicate with it
        m_pluginApiTwo.reset();
        std::vector<std::string> pluginsBeforeRemoval = { m_pluginOneName, m_pluginTwoName };
        std::vector<std::string> pluginsAfterRemoval = { m_pluginOneName };
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginOneName, "testpolicyone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsBeforeRemoval);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(m_pluginTwoName, "testpolicytwo"), 0);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsAfterRemoval);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestDoActionOnRegisteredPlugin) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testaction")).Times(1);
    std::thread applyAction([this]() { EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testaction"), 1); });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionNotSentToRegisteredPluginWithWrongAppId) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testaction")).Times(0);
    std::thread applyAction([this]() { EXPECT_EQ(m_pluginManagerPtr->queueAction("wrongappid", "testaction"), 0); });
    applyAction.join();
}

TEST_F(TestPluginManager, TestAppIdCanBeChangedForRegisteredPluginForAction) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionnotsent")).Times(0);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionsent")).Times(1);
    std::thread applyAction([this]() {
        EXPECT_EQ(m_pluginManagerPtr->queueAction("wrongappid", "testactionnotsent"), 0);
        std::vector<std::string> appIds;
        appIds.emplace_back("wrongappid");
        m_pluginManagerPtr->registerAndSetAppIds(m_pluginOneName, appIds, appIds);
        EXPECT_EQ(m_pluginManagerPtr->queueAction("wrongappid", "testactionsent"), 1);
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionOnTwoRegisteredPlugins) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactiontwo")).Times(1);
    std::thread applyAction([this]() {
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testactionone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginTwoName, "testactiontwo"), 1);
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionOnFailedPluginLeavesItInRegisteredPluginList) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactiontwo")).Times(0);
    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, isFile("/registry/plugin_two.json"))
        .WillOnce(Return(true))  // Register it
        .WillOnce(Return(true)); // Check it is still in Register
    std::thread applyPolicy([this]() {
        m_pluginManagerPtr->setDefaultConnectTimeout(10);
        m_pluginManagerPtr->setDefaultTimeout(10);
        // Register Plugin with management and add to proxy list
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
        // Shutdown plugin - management agent doesn't know it has gone away until it attempts to communicate with it
        m_pluginApiTwo.reset();
        std::vector<std::string> pluginsBeforeRemoval = { m_pluginOneName, m_pluginTwoName };
        std::vector<std::string> pluginsAfterRemoval = { m_pluginOneName, m_pluginTwoName };
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testactionone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsBeforeRemoval);
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginTwoName, "testactiontwo"), 0);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsAfterRemoval);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestDoActionOnPluginNoLongerInstalledRemovesItFromRegisteredPluginList) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactiontwo")).Times(0);
    auto& fileSystemMock = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystemMock, isFile("/registry/plugin_two.json"))
        .WillOnce(Return(true))   // Register it
        .WillOnce(Return(false)); // Check it is still in Register
    std::thread applyPolicy([this]() {
        m_pluginManagerPtr->setDefaultConnectTimeout(10);
        m_pluginManagerPtr->setDefaultTimeout(10);
        // Register Plugin with management and add to proxy list
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
        // Shutdown plugin - management agent doesn't know it has gone away until it attempts to communicate with it
        m_pluginApiTwo.reset();
        std::vector<std::string> pluginsBeforeRemoval = { m_pluginOneName, m_pluginTwoName };
        std::vector<std::string> pluginsAfterRemoval = { m_pluginOneName };
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testactionone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsBeforeRemoval);
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginTwoName, "testactiontwo"), 0);
        EXPECT_EQ(m_pluginManagerPtr->getRegisteredPluginNames(), pluginsAfterRemoval);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestDoActionOnTwoRegisteredPluginsInOneThread) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactiontwo")).Times(1);
    m_pluginApiTwo = m_mgmtCommon->createPluginAPI(m_pluginTwoName, m_mockedPluginApiCallback);
    EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testactionone"), 1);
    EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginTwoName, "testactiontwo"), 1);
}

TEST_F(TestPluginManager, TestGetStatusOnRegisteredPlugins) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getStatus(m_pluginOneName)).Times(1);
    std::thread getStatus([this]() { m_pluginManagerPtr->getStatus(m_pluginOneName); });
    getStatus.join();
}

TEST_F(TestPluginManager, TestGetStatusOnUnregisteredPluginThrows) // NOLINT
{
    EXPECT_THROW( // NOLINT
        m_pluginManagerPtr->getStatus("plugin_not_registered"),
        ManagementAgent::PluginCommunication::IPluginCommunicationException); // NOLINT
}

TEST_F(TestPluginManager, TestGetStatusOnRemovedPluginThrows) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getStatus(m_pluginOneName)).Times(1);
    std::thread getStatus([this]() {
        m_pluginManagerPtr->getStatus(m_pluginOneName);
        m_pluginManagerPtr->removePlugin(m_pluginOneName);
    });
    getStatus.join();
    EXPECT_THROW(                                       // NOLINT
        m_pluginManagerPtr->getStatus(m_pluginOneName), // NOLINT
        ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetTelemetryOnRegisteredPlugins) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getTelemetry()).Times(1).WillOnce(Return("telemetryContent"));
    std::thread getTelemetry(
        [this]() { ASSERT_EQ(m_pluginManagerPtr->getTelemetry(m_pluginOneName), "telemetryContent"); });
    getTelemetry.join();
}

TEST_F(TestPluginManager, TestGetTelemetryOnUnregisteredPluginThrows) // NOLINT
{
    EXPECT_THROW(                                                  // NOLINT
        m_pluginManagerPtr->getTelemetry("plugin_not_registered"), // NOLINT
        ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetTelemetryOnRemovedPluginThrows) // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getTelemetry()).Times(1);
    std::thread getTelemetry([this]() {
        m_pluginManagerPtr->getTelemetry(m_pluginOneName);
        m_pluginManagerPtr->removePlugin(m_pluginOneName);
    });
    getTelemetry.join();
    EXPECT_THROW(                                          // NOLINT
        m_pluginManagerPtr->getTelemetry(m_pluginOneName), // NOLINT
        ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestRegistrationOfASeccondPluginWithTheSameName) // NOLINT
{
    auto secondMockedPluginApiCallback = std::make_shared<StrictMock<MockedPluginApiCallback>>();
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionone")).Times(1);
    EXPECT_CALL(*secondMockedPluginApiCallback, queueAction("testaction_after_re-registration")).Times(1);
    std::thread secondRegistration([this, &secondMockedPluginApiCallback]() {
        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testactionone"), 1);

        // the system will fail to create a plugin to bind to the same address.
        ASSERT_THROW(
            m_mgmtCommon->createPluginAPI(m_pluginOneName, secondMockedPluginApiCallback),
            Common::PluginApi::ApiException); // NOLINT
        // shutdown the plugin
        m_pluginApi.reset();

        // register the plugin again.
        // it usually can take a small time to clean up the plugin socket
        int count = 3;
        while (--count > 0)
        {
            try
            {
                m_pluginApi = m_mgmtCommon->createPluginAPI(m_pluginOneName, secondMockedPluginApiCallback);
                // on success of creating the m_pluginApi... carry on.
                break;
            }
            catch (Common::PluginApi::ApiException& ex)
            {
                std::string reason = ex.what();
                EXPECT_THAT(reason, HasSubstr("Failed to bind"));
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            catch (std::exception& ex)
            {
                ASSERT_FALSE(true) << ex.what();
            }
        }
        ASSERT_TRUE(m_pluginApi) << "Failed to create a new plugin api binding to the same address. ";

        EXPECT_EQ(m_pluginManagerPtr->queueAction(m_pluginOneName, "testaction_after_re-registration"), 1);
    });
    secondRegistration.join();
}
