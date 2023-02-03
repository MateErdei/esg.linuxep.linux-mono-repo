// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/pluginimpl/PluginAdapter.h"

#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockApiBaseServices.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class PluginAdapterTests : public MemoryAppenderUsingTests
{
public:
    PluginAdapterTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
    void SetUp() override
    {
        m_taskQueue = std::make_shared<Plugin::QueueTask>();
        m_callback = std::make_shared<Plugin::PluginCallback>(m_taskQueue);
    }

    std::shared_ptr<Plugin::QueueTask> m_taskQueue;
    std::shared_ptr<Plugin::PluginCallback> m_callback;
};

TEST_F(PluginAdapterTests, actionIsLoggedWhenSent) // NOLINT
{

    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    Plugin::PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback);

    m_taskQueue->push(Plugin::Task{ Plugin::Task::TaskType::ACTION, "action here" });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("action here"));

}
