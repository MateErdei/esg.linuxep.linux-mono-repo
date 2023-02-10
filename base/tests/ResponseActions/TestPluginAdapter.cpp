// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponsePlugin/PluginAdapter.h"

#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
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
        m_taskQueue = std::make_shared<ResponsePlugin::TaskQueue>();
        m_callback = std::make_shared<ResponsePlugin::PluginCallback>(m_taskQueue);
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
        Tests::restoreFilePermissions();
    }

    void mockSendingResponse()
    {
        auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
        EXPECT_CALL(*mockFileSystem, writeFile(_,_)).Times(1);
        EXPECT_CALL(*mockFileSystem, moveFile(_,_)).Times(1);
        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
        EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
        EXPECT_CALL(*mockFilePermissions, chown(_, _, _)).WillRepeatedly(Return());
    }
    std::shared_ptr<ResponsePlugin::TaskQueue> m_taskQueue;
    std::shared_ptr<ResponsePlugin::PluginCallback> m_callback;
};

TEST_F(PluginAdapterTests, invalidActionIsThrownAway)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);

    ResponsePlugin::PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, "{\"type\":\"string\"}" });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);
    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Unknown action throwing it away"));

}

TEST_F(PluginAdapterTests, UploadFileActionTriggersRun)
{
    auto mockBaseService = std::make_unique<::testing::StrictMock<MockApiBaseServices>>();
    MockApiBaseServices* mockBaseServicePtr = mockBaseService.get();
    ASSERT_NE(mockBaseServicePtr, nullptr);
    mockSendingResponse();

    ResponsePlugin::PluginAdapter pluginAdapter(
        m_taskQueue, std::move(mockBaseService), m_callback);

    m_taskQueue->push(ResponsePlugin::Task{ ResponsePlugin::Task::TaskType::ACTION, "{\"type\":\"sophos.mgt.action.UploadFile\"}" });
    m_taskQueue->pushStop();
    UsingMemoryAppender recorder(*this);

    pluginAdapter.mainLoop();

    EXPECT_TRUE(appenderContains("Running upload"));

}