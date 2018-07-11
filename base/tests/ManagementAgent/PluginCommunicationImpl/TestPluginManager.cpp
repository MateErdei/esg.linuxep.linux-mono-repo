
#include "ManagementAgent/PluginCommunicationImpl/PluginManager.h"
#include "ManagementAgent/PluginCommunicationImpl/PluginProxy.h"
#include "ManagementAgent/PluginCommunication/IPluginCommunicationException.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"

#include "Common/ZeroMQWrapper/IContext.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "tests/Common/PluginApiImpl/MockedApplicationPathManager.h"
#include "tests/Common/PluginApiImpl/MockedPluginApiCallback.h"

#include <thread>
using ManagementAgent::PluginCommunicationImpl::PluginProxy;

class TestPluginManager : public ::testing::Test
{
public:
    TestPluginManager()
    {
        plugin_one_name = "plugin_one";
        plugin_two_name = "plugin_two";
        MockedApplicationPathManager *mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));
        ON_CALL(*mockApplicationPathManager, getManagementAgentSocketAddress()).WillByDefault(
                Return("inproc:///tmp/management.ipc"));
        m_pluginManagerPtr = std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager>(
                new ManagementAgent::PluginCommunicationImpl::PluginManager());

        ON_CALL(*mockApplicationPathManager, getPluginSocketAddress(plugin_one_name)).WillByDefault(
                Return("inproc:///tmp/plugin_one"));
        ON_CALL(*mockApplicationPathManager, getPluginSocketAddress(plugin_two_name)).WillByDefault(
                Return("inproc:///tmp/plugin_two"));
        m_mockedPluginApiCallback = std::make_shared<StrictMock<MockedPluginApiCallback>>();
        m_mgmtCommon = std::unique_ptr<Common::PluginApiImpl::PluginResourceManagement>(
                new Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext()));
        m_pluginApi = m_mgmtCommon->createPluginAPI(plugin_one_name, m_mockedPluginApiCallback);
    }

    ~TestPluginManager() = default;

    std::string plugin_one_name;
    std::string plugin_two_name;
    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager> m_pluginManagerPtr;
    std::unique_ptr<Common::PluginApiImpl::PluginResourceManagement> m_mgmtCommon;
    std::shared_ptr<MockedPluginApiCallback> m_mockedPluginApiCallback;
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_pluginApi;
    std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_pluginApiTwo;

};

TEST_F(TestPluginManager, TestApplyPolicyOnRegisteredPlugin)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicy")).Times(1);
    std::thread applyPolicy([this]() {
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(plugin_one_name, "testpolicy"), 1);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyNotSentToRegisteredPluginWithWrongAppId)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicy")).Times(0);
    std::thread applyPolicy([this]() {
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicy"), 0);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestAppIdCanBeChangedForRegisteredPluginForPolicy)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicynotsent")).Times(0);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicysent")).Times(1);
    std::thread applyPolicy([this]() {
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicynotsent"), 0);
        std::vector<std::string> appIds;
        appIds.emplace_back("wrongappid");
        m_pluginManagerPtr->setAppIds(plugin_one_name, appIds, appIds);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicysent"), 1);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyOnTwoRegisteredPlugins)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicyone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicytwo")).Times(1);
    std::thread applyPolicy([this]() {
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(plugin_two_name, m_mockedPluginApiCallback);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(plugin_one_name, "testpolicyone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->applyNewPolicy(plugin_two_name, "testpolicytwo"), 1);
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestDoActionOnRegisteredPlugin)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testaction")).Times(1);
    std::thread applyAction([this]() {
        EXPECT_EQ(m_pluginManagerPtr->queueAction(plugin_one_name, "testaction"), 1);
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionNotSentToRegisteredPluginWithWrongAppId)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testaction")).Times(0);
    std::thread applyAction([this]() {
        EXPECT_EQ(m_pluginManagerPtr->queueAction("wrongappid", "testaction"), 0);
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestAppIdCanBeChangedForRegisteredPluginForAction)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionnotsent")).Times(0);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionsent")).Times(1);
    std::thread applyAction([this]() {
        EXPECT_EQ(m_pluginManagerPtr->queueAction("wrongappid", "testactionnotsent"), 0);
        std::vector<std::string> appIds;
        appIds.emplace_back("wrongappid");
        m_pluginManagerPtr->setAppIds(plugin_one_name, appIds, appIds);
        EXPECT_EQ(m_pluginManagerPtr->queueAction("wrongappid", "testactionsent"), 1);
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionOnTwoRegisteredPlugins)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactionone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, queueAction("testactiontwo")).Times(1);
    std::thread applyAction([this]() {
        m_pluginApiTwo = m_mgmtCommon->createPluginAPI(plugin_two_name, m_mockedPluginApiCallback);
        EXPECT_EQ(m_pluginManagerPtr->queueAction(plugin_one_name, "testactionone"), 1);
        EXPECT_EQ(m_pluginManagerPtr->queueAction(plugin_two_name, "testactiontwo"), 1);
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestGetStatusOnRegisteredPlugins)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getStatus(plugin_one_name)).Times(1);
    std::thread getStatus([this]() {
        m_pluginManagerPtr->getStatus(plugin_one_name);
    });
    getStatus.join();
}

TEST_F(TestPluginManager, TestGetStatusOnUnregisteredPluginThrows)
{
    EXPECT_THROW(m_pluginManagerPtr->getStatus("plugin_not_registered"),
                 ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetStatusOnRemovedPluginThrows)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getStatus(plugin_one_name)).Times(1);
    std::thread getStatus([this]() {
        m_pluginManagerPtr->getStatus(plugin_one_name);
        m_pluginManagerPtr->removePlugin(plugin_one_name);
    });
    getStatus.join();
    EXPECT_THROW(m_pluginManagerPtr->getStatus(plugin_one_name),
                 ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetTelemetryOnRegisteredPlugins)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getTelemetry()).Times(1).WillOnce(Return("telemetryContent"));
    std::thread getTelemetry([this]() {
        ASSERT_EQ(m_pluginManagerPtr->getTelemetry(plugin_one_name), "telemetryContent");
    });
    getTelemetry.join();
}

TEST_F(TestPluginManager, TestGetTelemetryOnUnregisteredPluginThrows)
{
    EXPECT_THROW(m_pluginManagerPtr->getTelemetry("plugin_not_registered"),
                 ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetTelemetryOnRemovedPluginThrows)  // NOLINT
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getTelemetry()).Times(1);
    std::thread getTelemetry([this]() {
        m_pluginManagerPtr->getTelemetry(plugin_one_name);
        m_pluginManagerPtr->removePlugin(plugin_one_name);
    });
    getTelemetry.join();
    EXPECT_THROW(m_pluginManagerPtr->getTelemetry(plugin_one_name),
                 ManagementAgent::PluginCommunication::IPluginCommunicationException);
}