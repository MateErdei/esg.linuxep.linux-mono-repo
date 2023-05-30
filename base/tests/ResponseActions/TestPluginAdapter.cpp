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
    UsingMemoryAppender recorder(*this);

    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION,
                                            R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})" });
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Running action: "));
}

TEST_F(PluginAdapterTests, TestActionsRunInOrder)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    ASSERT_NE(mockBaseService.get(), nullptr);

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
    UsingMemoryAppender recorder(*this);

    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    auto runner = std::make_unique<TestableActionRunner>();
    runner.get()->setIsRunning(true);
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION,
                                            R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})" });
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();
    EXPECT_FALSE(appenderContains("Running action: "));
}

TEST_F(PluginAdapterTests, ActionIsNotRunWhenTimeOutIsNegative)
{
    UsingMemoryAppender recorder(*this);
    std::string action = R"({"type":"sophos.mgt.action.UploadFile","timeout":-1000})";
    std::string expectedMsg = "Action does not have a valid timeout set discarding it: " + action;

    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    ASSERT_NE(mockBaseService.get(), nullptr);

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION,
                                            action});
    m_taskQueue->pushStop();
    pluginAdapter.mainLoop();
    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(PluginAdapterTests, ActionIsNotRunWhenTimeOutIsInvalid)
{
    UsingMemoryAppender recorder(*this);
    std::string action_timeout_str = R"({"type":"sophos.mgt.action.UploadFile","timeout":"1000"})";
    std::string action_timeout_bool = R"({"type":"sophos.mgt.action.UploadFile","timeout":true})";
    std::string expectedMsg_str = "Action does not have a valid timeout set discarding it: " + action_timeout_str;
    std::string expectedMsg_bool = "Action does not have a valid timeout set discarding it: " + action_timeout_bool;

    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    ASSERT_NE(mockBaseService.get(), nullptr);

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, action_timeout_str});
    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, action_timeout_bool});
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains(expectedMsg_str));
    EXPECT_TRUE(appenderContains(expectedMsg_bool));
}

TEST_F(PluginAdapterTests, ActionIsRunWhenTimeOutIsFloat)
{
    UsingMemoryAppender recorder(*this);
    std::string action = R"({"type":"sophos.mgt.action.UploadFile","timeout":1000.56})";
    std::string expectedMsg_str = "Running action: ";

    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    ASSERT_NE(mockBaseService.get(), nullptr);

    auto runner = std::make_unique<TestableActionRunner>();
    ResponsePlugin::PluginAdapter pluginAdapter(m_taskQueue, std::move(mockBaseService), std::move(runner), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, action});
    m_taskQueue->pushStop();

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains(expectedMsg_str));
}