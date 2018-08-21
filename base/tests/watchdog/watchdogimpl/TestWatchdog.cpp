/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <watchdog/watchdogimpl/Watchdog.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <watchdog/watchdogimpl/LoggingSetup.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace
{
    class TestWatchdog
            : public ::testing::Test
    {
        std::unique_ptr<watchdog::watchdogimpl::LoggingSetup> m_loggingSetup;
    public:
        TestWatchdog()
                : m_loggingSetup(
                std::unique_ptr<watchdog::watchdogimpl::LoggingSetup>(new watchdog::watchdogimpl::LoggingSetup(1)))
        {}
    };

    class TestableWatchdog
            : public watchdog::watchdogimpl::Watchdog
    {
    public:
        explicit TestableWatchdog(Common::ZeroMQWrapper::IContextSharedPtr context)
            : watchdog::watchdogimpl::Watchdog(std::move(context))
        {}

        explicit TestableWatchdog() = default;

        void callSetupIpc()
        {
            setupSocket();
        }

        void callHandleSocketRequest()
        {
            handleSocketRequest();
        };

        /**
         * Give access to the context so that we can share connections
         * @return
         */
        Common::ZeroMQWrapper::IContext& context()
        {
            return *m_context;
        }

        void addPlugin(const std::string& pluginName)
        {
            Common::PluginRegistryImpl::PluginInfo info;
            info.setPluginName(pluginName);
            m_pluginProxies.emplace_back(info);
        }
    };
}

TEST_F(TestWatchdog, Construction) // NOLINT
{
    EXPECT_NO_THROW(watchdog::watchdogimpl::Watchdog watchdog); // NOLINT
    EXPECT_NO_THROW(TestableWatchdog watchdog2); // NOLINT
}

TEST_F(TestWatchdog, ipcStartup) // NOLINT
{
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc","inproc://ipcStartup");
    TestableWatchdog watchdog;
    EXPECT_NO_THROW(watchdog.callSetupIpc()); // NOLINT
}

TEST_F(TestWatchdog, stopPluginViaIPC_missing_plugin) // NOLINT
{
    const std::string IPC_ADDRESS="inproc://stopPluginViaIPC";
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc",IPC_ADDRESS);
    Common::ZeroMQWrapper::IContextSharedPtr context(Common::ZeroMQWrapper::createContext());
    TestableWatchdog watchdog(context);
    watchdog.callSetupIpc();
    Common::ZeroMQWrapper::ISocketRequesterPtr requester = context->getRequester();
    requester->connect(IPC_ADDRESS);
    requester->write({"STOP","mcsrouter"});

    watchdog.callHandleSocketRequest();

    Common::ZeroMQWrapper::IReadable::data_t result = requester->read();
    EXPECT_EQ(result.at(0),"Error: Plugin not found");
}

TEST_F(TestWatchdog, stopPluginViaIPC_test_plugin) // NOLINT
{
    const std::string IPC_ADDRESS="inproc://stopPluginViaIPC_test_plugin";
    Common::ApplicationConfiguration::applicationConfiguration().setData("watchdog.ipc",IPC_ADDRESS);
    Common::ZeroMQWrapper::IContextSharedPtr context(Common::ZeroMQWrapper::createContext());
    TestableWatchdog watchdog(context);
    watchdog.callSetupIpc();

    watchdog.addPlugin("mcsrouter");

    Common::ZeroMQWrapper::ISocketRequesterPtr requester = context->getRequester();
    requester->connect(IPC_ADDRESS);
    requester->write({"STOP","mcsrouter"});

    watchdog.callHandleSocketRequest();

    Common::ZeroMQWrapper::IReadable::data_t result = requester->read();
    EXPECT_EQ(result.at(0),"OK");

}

class TestC
{
public:
    explicit TestC(int pos)
        : m_running(false),m_pos(pos)
    {
    }

    TestC(TestC&& other) noexcept
        : m_running(false)
    {
        std::swap(m_running, other.m_running);
        m_pos = 5;
    }

    ~TestC()
    {
        std::cerr << "Deleting "<<m_pos << " running="<< m_running << std::endl;
        EXPECT_FALSE(m_running);
    }

    TestC& operator=(TestC&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        std::swap(m_running, other.m_running);
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