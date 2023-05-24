// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponsePlugin/IActionRunner.h"
#include "modules/ResponseActions/ResponsePlugin/PluginAdapter.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockApiBaseServices.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>

class TestableActionRunner : public ResponsePlugin::IActionRunner
{
public:
    void setIsRunning(bool running) { isRunning = running; }
    void runAction(const std::string& action, const std::string& correlationId, const std::string& type, int timeout)
    {
        std::cout << "Running Action"
                  << " " << action << ""
                  << " " << correlationId << ""
                  << " " << timeout << ""
                  << " " << type << "" << std::endl;
    }
    void killAction() { std::cout << "Running Killing" << std::endl; }
    bool getIsRunning() { return isRunning; }

private:
    bool isRunning = false;
};
class PluginAdapterTests : public MemoryAppenderUsingTests
{
public:
    PluginAdapterTests() : MemoryAppenderUsingTests("responseactions") {}
    void SetUp() override
    {
        m_taskQueue = std::make_shared<ResponsePlugin::TaskQueue>();
        m_callback = std::make_shared<ResponsePlugin::PluginCallback>(m_taskQueue);
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
        Tests::restoreFilePermissions();
    }

    std::shared_ptr<ResponsePlugin::TaskQueue> m_taskQueue;
    std::shared_ptr<ResponsePlugin::PluginCallback> m_callback;
};

TEST_F(PluginAdapterTests, emptyActionTypeIsThrownAway)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    std::string action = R"({"type":"","timeout":1000})";
    std::string expectedStr ("Throwing away unknown action: " + action);

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(
        ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, action });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains(expectedStr));
}

TEST_F(PluginAdapterTests, ActionWithMissingTimeoutIsThrownAway)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, R"({"type":"something"})" });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Missing either type or timeout field"));
}

TEST_F(PluginAdapterTests, UploadFileActionTriggersRun)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION,
                                            R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})" });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Running action: "));
}

TEST_F(PluginAdapterTests, TestActionsRunInOrder)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{
        ResponsePlugin::Task::TaskType::ACTION, R"({"type":"a","timeout":1})", "CORE", "id1" });
    m_taskQueue->push(ResponsePlugin::Task{
        ResponsePlugin::Task::TaskType::ACTION, R"({"type":"a","timeout":1})", "CORE", "id2" });
    m_taskQueue->push(ResponsePlugin::Task{
        ResponsePlugin::Task::TaskType::ACTION, R"({"type":"a","timeout":1})", "CORE", "id3" });
    m_taskQueue->pushStop();
    testing::internal::CaptureStderr();

    pluginAdapter.mainLoop();
    std::string logMessage = internal::GetCapturedStderr();
    size_t pos1 = logMessage.find("Running action: id1");
    size_t pos2 = logMessage.find("Running action: id2");
    size_t pos3 = logMessage.find("Running action: id3");
    EXPECT_NE(std::string::npos, pos1);
    EXPECT_NE(std::string::npos, pos2);
    EXPECT_NE(std::string::npos, pos3);
    EXPECT_GT(pos3, pos2);
    EXPECT_GT(pos2, pos1);
}

TEST_F(PluginAdapterTests, UploadFileActionDoesNotTriggersRunWhenActionIsRunning)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    auto runner = std::make_unique<TestableActionRunner>();
    runner.get()->setIsRunning(true);
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION,
                                            R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})" });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);

    pluginAdapter.mainLoop();

    EXPECT_FALSE(appenderContains("Running action: "));
}