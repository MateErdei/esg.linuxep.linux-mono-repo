// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/OnAccessStatusMonitor.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Test code
#include "common/StatusFile.h"
#include "tests/common/TestSpecificDirectory.h"

#include <gtest/gtest.h>

namespace
{
    namespace fs = sophos_filesystem;
    class TestOnAccessStatusMonitor : public PluginMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_testDir = test_common::createTestSpecificDirectory();
            fs::current_path(m_testDir);
            auto var = m_testDir / "var";
            fs::create_directories(var);
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", m_testDir);

            m_taskQueue = std::make_shared<Plugin::TaskQueue>();
            m_pluginCallback = std::make_shared<Plugin::PluginCallback>(m_taskQueue);
        }

        void TearDown() override
        {
            test_common::removeTestSpecificDirectory(m_testDir);
        }

        fs::path m_testDir;
        std::shared_ptr<Plugin::TaskQueue> m_taskQueue;
        std::shared_ptr<Plugin::PluginCallback> m_pluginCallback;
    };
}

TEST_F(TestOnAccessStatusMonitor, construction)
{
    Plugin::OnAccessStatusMonitor monitor(m_pluginCallback);
}

TEST_F(TestOnAccessStatusMonitor, startThread)
{
    auto monitor = std::make_shared<Plugin::OnAccessStatusMonitor>(m_pluginCallback);

    {
        common::ThreadRunner runner(monitor, "OAMonitor", true);
        runner.requestStopIfNotStopped();
    }

    ASSERT_TRUE(m_taskQueue->empty()); // No event if on-access disabled
}

TEST_F(TestOnAccessStatusMonitor, startThreadWithStatusFileDisabled)
{
    auto path = Plugin::getOnAccessStatusPath();
    auto statusFile = std::make_shared<common::StatusFile>(path);
    statusFile->disabled();
    auto monitor = std::make_shared<Plugin::OnAccessStatusMonitor>(m_pluginCallback);

    {
        common::ThreadRunner runner(monitor, "OAMonitor", true);
        runner.requestStopIfNotStopped();
    }

    ASSERT_TRUE(m_taskQueue->empty()); // No event if on-access disabled
}

TEST_F(TestOnAccessStatusMonitor, startThreadWithStatusFileEnabled)
{
    auto path = Plugin::getOnAccessStatusPath();
    auto statusFile = std::make_shared<common::StatusFile>(path);
    statusFile->enabled();
    auto monitor = std::make_shared<Plugin::OnAccessStatusMonitor>(m_pluginCallback);

    {
        common::ThreadRunner runner(monitor, "OAMonitor", true);
        runner.requestStopIfNotStopped();
    }

    ASSERT_FALSE(m_taskQueue->empty());
    auto task = m_taskQueue->pop();
    ASSERT_EQ(task.taskType, Plugin::Task::TaskType::SendStatus);
    EXPECT_THAT(task.Content, testing::HasSubstr("<on-access>true</on-access>"));
}

TEST_F(TestOnAccessStatusMonitor, disableSendsEventIfAfterEnable)
{
    using namespace std::chrono_literals;
    auto path = Plugin::getOnAccessStatusPath();
    auto statusFile = std::make_shared<common::StatusFile>(path);
    statusFile->enabled();
    auto monitor = std::make_shared<Plugin::OnAccessStatusMonitor>(m_pluginCallback);

    {
        common::ThreadRunner runner(monitor, "OAMonitor", true);

        Plugin::Task task;
        auto timeout = Plugin::TaskQueue::clock_t::now() + 20ms;
        bool found = m_taskQueue->pop(task, timeout);
        ASSERT_TRUE(found);
        ASSERT_EQ(task.taskType, Plugin::Task::TaskType::SendStatus);
        EXPECT_THAT(task.Content, testing::HasSubstr("<on-access>true</on-access>"));

        statusFile->disabled();
        timeout = Plugin::TaskQueue::clock_t::now() + 700s;
        found = m_taskQueue->pop(task, timeout);
        ASSERT_TRUE(found);
        ASSERT_EQ(task.taskType, Plugin::Task::TaskType::SendStatus);
        EXPECT_THAT(task.Content, testing::HasSubstr("<on-access>false</on-access>"));

        runner.requestStopIfNotStopped();
    }
}
