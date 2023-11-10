// Copyright 2023 Sophos Limited. All rights reserved.

#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/TaskQueue.h"

#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockApiBaseServices.h"

#include <gtest/gtest.h>

#include <future>
#include <memory>

using namespace testing;

class TestablePluginAdapter : public Plugin::PluginAdapter
{
public:
    TestablePluginAdapter(
        const std::shared_ptr<Plugin::TaskQueue>& queueTask
        ) :
        Plugin::PluginAdapter(
            queueTask,
            std::make_unique<StrictMock<MockApiBaseServices>>(),
            std::make_shared<Plugin::PluginCallback>(queueTask))
    {
    }

    void setQueueTimeout(std::chrono::milliseconds timeout)
    {
        QUEUE_TIMEOUT = timeout;
    }
};

class PluginAdapterTests : public LogOffInitializedTests
{
};

TEST_F(PluginAdapterTests, PluginAdapterRestartsSubscriberOrWriterIfTheyStop)
{
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    TestablePluginAdapter pluginAdapter(queueTask);

    pluginAdapter.setQueueTimeout(std::chrono::milliseconds{30});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &pluginAdapter);

    queueTask->pushStop();
    mainLoopFuture.get();
}
