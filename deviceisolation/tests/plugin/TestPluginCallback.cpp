// Copyright 2023 Sophos Limited. All rights reserved.

#include "pluginimpl/ApplicationPaths.h"
#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/TaskQueue.h"

#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

class TestPluginCallback : public LogOffInitializedTests
{
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }

};

TEST_F(TestPluginCallback, testGetHealthReturns0WhenAllFactorsHealthy)
{
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask);

    ASSERT_EQ(sharedPluginCallBack.getHealth(), "{'Health': 0}");
    auto telemetryJson = sharedPluginCallBack.getTelemetry();
    EXPECT_TRUE(telemetryJson.find("health\":0") != std::string::npos);
}

class PluginCallbackWithMockedHealthInner : public  Plugin::PluginCallback {
public:
    MOCK_METHOD0(getHealthInner, uint());

    PluginCallbackWithMockedHealthInner(std::shared_ptr<Plugin::TaskQueue> task)
            : Plugin::PluginCallback(task)
            {};
};

TEST_F(TestPluginCallback, testGetTelemetryCallsGetHealthInner)
{
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto sharedPluginCallBack = PluginCallbackWithMockedHealthInner(queueTask);

    EXPECT_CALL(sharedPluginCallBack, getHealthInner()).Times(1);
    sharedPluginCallBack.getTelemetry();
}