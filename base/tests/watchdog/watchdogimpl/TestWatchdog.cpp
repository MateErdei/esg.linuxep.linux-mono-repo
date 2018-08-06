/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <watchdog/watchdogimpl/Watchdog.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace
{
    class TestWatchdog
            : public ::testing::Test
    {
    public:
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