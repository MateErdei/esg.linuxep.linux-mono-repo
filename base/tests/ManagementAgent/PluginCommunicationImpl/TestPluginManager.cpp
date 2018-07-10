
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginProxy.h>
#include <ManagementAgent/PluginCommunication/IPluginCommunicationException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <thread>
#include "Common/ZeroMQWrapper/IContext.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../../Common/PluginApiImpl/MockedApplicationPathManager.h"
#include "../../Common/PluginApiImpl/MockedPluginApiCallback.h"

using ManagementAgent::PluginCommunicationImpl::PluginProxy;

class TestPluginManager : public ::testing::Test
{
public:
    TestPluginManager()
    {
        MockedApplicationPathManager *mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));
        ON_CALL(*mockApplicationPathManager, getManagementAgentSocketAddress()).WillByDefault(
                Return("inproc:///tmp/management.ipc"));
        m_pluginManagerPtr = std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager>(
                new ManagementAgent::PluginCommunicationImpl::PluginManager());

        ON_CALL(*mockApplicationPathManager, getPluginSocketAddress("plugin_one")).WillByDefault(
                Return("inproc:///tmp/plugin_one"));
        ON_CALL(*mockApplicationPathManager, getPluginSocketAddress("plugin_two")).WillByDefault(
                Return("inproc:///tmp/plugin_two"));
        m_mockedPluginApiCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();
    }

    ~TestPluginManager()
    {}

    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager> m_pluginManagerPtr;
    std::shared_ptr<MockedPluginApiCallback> m_mockedPluginApiCallback;
    std::unique_ptr<Common::PluginApi::IPluginApi> m_pluginApi;
    std::unique_ptr<Common::PluginApi::IPluginApi> m_pluginApiTwo;
};

TEST_F(TestPluginManager, TestApplyPolicyOnRegisteredPlugin)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicy")).Times(1);
    std::thread applyPolicy([&]() {
        Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->applyNewPolicy("plugin_one", "testpolicy");
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyNotSentToRegisteredPluginWithWrongAppId)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicy")).Times(0);
    std::thread applyPolicy([&]() {
        Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicy");
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestAppIdCanBeChangedForRegisteredPluginForPolicy)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicynotsent")).Times(0);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicysent")).Times(1);
    std::thread applyPolicy([&]() {
        Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicynotsent");
        std::vector<std::string> appIds;
        appIds.emplace_back("wrongappid");
        m_pluginManagerPtr->setAppIds("plugin_one", appIds);
        m_pluginManagerPtr->applyNewPolicy("wrongappid", "testpolicysent");
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestApplyPolicyOnTwoRegisteredPlugins)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicyone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, applyNewPolicy("testpolicytwo")).Times(1);
    Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
    std::thread applyPolicy([&]() {
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginApiTwo=mgmtCommon.createPluginAPI("plugin_two", m_mockedPluginApiCallback);
        m_pluginManagerPtr->applyNewPolicy("plugin_one", "testpolicyone");
        m_pluginManagerPtr->applyNewPolicy("plugin_two", "testpolicytwo");
    });
    applyPolicy.join();
}

TEST_F(TestPluginManager, TestDoActionOnRegisteredPlugin)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, doAction("testaction")).Times(1);
    std::thread applyAction([&]() {
        Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->doAction("plugin_one", "testaction");
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionNotSentToRegisteredPluginWithWrongAppId)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, doAction("testaction")).Times(0);
    std::thread applyAction([&]() {
        Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->doAction("wrongappid", "testaction");
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestAppIdCanBeChangedForRegisteredPluginForAction)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, doAction("testactionnotsent")).Times(0);
    EXPECT_CALL(*m_mockedPluginApiCallback, doAction("testactionsent")).Times(1);
    std::thread applyAction([&]() {
        Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->doAction("wrongappid", "testactionnotsent");
        std::vector<std::string> appIds;
        appIds.emplace_back("wrongappid");
        m_pluginManagerPtr->setAppIds("plugin_one", appIds);
        m_pluginManagerPtr->doAction("wrongappid", "testactionsent");
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestDoActionOnTwoRegisteredPlugins)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, doAction("testactionone")).Times(1);
    EXPECT_CALL(*m_mockedPluginApiCallback, doAction("testactiontwo")).Times(1);
    Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
    std::thread applyAction([&]() {
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginApiTwo=mgmtCommon.createPluginAPI("plugin_two", m_mockedPluginApiCallback);
        m_pluginManagerPtr->doAction("plugin_one", "testactionone");
        m_pluginManagerPtr->doAction("plugin_two", "testactiontwo");
    });
    applyAction.join();
}

TEST_F(TestPluginManager, TestGetStatusOnRegisteredPlugins)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getStatus()).Times(1);
    Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
    std::thread getStatus([&]() {
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->getStatus("plugin_one");
    });
    getStatus.join();
}

TEST_F(TestPluginManager, TestGetStatusOnUnregisteredPluginThrows)
{
    EXPECT_THROW(m_pluginManagerPtr->getStatus("plugin_one"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetStatusOnRemovedPluginThrows)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getStatus()).Times(1);
    Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
    std::thread getStatus([&]() {
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->getStatus("plugin_one");
        m_pluginManagerPtr->removePlugin("plugin_one");
    });
    getStatus.join();
    EXPECT_THROW(m_pluginManagerPtr->getStatus("plugin_one"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetTelemetryOnRegisteredPlugins)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getTelemetry()).Times(1);
    Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
    std::thread getTelemetry([&]() {
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->getTelemetry("plugin_one");
    });
    getTelemetry.join();
}

TEST_F(TestPluginManager, TestGetTelemetryOnUnregisteredPluginThrows)
{
    EXPECT_THROW(m_pluginManagerPtr->getTelemetry("plugin_one"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginManager, TestGetTelemetryOnRemovedPluginThrows)
{
    EXPECT_CALL(*m_mockedPluginApiCallback, getTelemetry()).Times(1);
    Common::PluginApiImpl::PluginResourceManagement mgmtCommon = Common::PluginApiImpl::PluginResourceManagement(&m_pluginManagerPtr->getSocketContext());
    std::thread getTelemetry([&]() {
        m_pluginApi=mgmtCommon.createPluginAPI("plugin_one", m_mockedPluginApiCallback);
        m_pluginManagerPtr->getTelemetry("plugin_one");
        m_pluginManagerPtr->removePlugin("plugin_one");
    });
    getTelemetry.join();
    EXPECT_THROW(m_pluginManagerPtr->getTelemetry("plugin_one"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}