// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/FileSystem/IPermissionDeniedException.h"
#include "Common/TaskQueueImpl/TaskProcessorImpl.h"
#include "Common/TaskQueueImpl/TaskQueueImpl.h"
#include "ManagementAgent/EventReceiverImpl/EventReceiverImpl.h"
#include "ManagementAgent/PluginCommunicationImpl/PluginManager.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockPoller.h"
#include "tests/Common/Helpers/MockPollerFactory.h"
#include "tests/Common/Helpers/MockZmqContext.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ManagementAgent::EventReceiverImpl::EventReceiverImpl;
using ManagementAgent::PluginCommunicationImpl::PluginManager;

namespace
{
    class TestPluginManagerQueueAction : public MemoryAppenderUsingTests
    {
    protected:
        TestPluginManagerQueueAction() : MemoryAppenderUsingTests("managementagent"), usingMemoryAppender_{ *this } {}

        void TearDown() override
        {
            Common::ZeroMQWrapper::restorePollerFactory();
            Tests::restoreFileSystem();
            Tests::restoreFilePermissions();
        }

        std::unique_ptr<MockedApplicationPathManager> mockedApplicationPathManager_ =
            std::make_unique<MockedApplicationPathManager>();
        std::unique_ptr<MockFileSystem> mockFileSystem_ = std::make_unique<MockFileSystem>();
        std::unique_ptr<MockFilePermissions> mockFilePermissions_ = std::make_unique<MockFilePermissions>();
        UsingMemoryAppender usingMemoryAppender_;

        void enterOutbreakMode(EventReceiverImpl& eventReceiver)
        {
            for (int i = 0; i < 100; ++i)
            {
                eventReceiver.receivedSendEvent("appId", R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection")");
            }
            ASSERT_TRUE(
                waitForLog("INFO - Generating Outbreak notification with UUID=5df69683-a5a2-5d96-897d-06f9c4c8c7bf"));
            ASSERT_TRUE(eventReceiver.outbreakMode());
        }
    };

    std::tuple<
        std::shared_ptr<PluginManager>,
        std::shared_ptr<EventReceiverImpl>,
        std::shared_ptr<Common::TaskQueueImpl::TaskProcessorImpl>>
    createPluginManager()
    {
        auto taskQueue = std::make_shared<Common::TaskQueueImpl::TaskQueueImpl>();
        auto taskProcessor = std::make_shared<Common::TaskQueueImpl::TaskProcessorImpl>(taskQueue);

        auto eventReceiver = std::make_shared<EventReceiverImpl>(taskQueue);

        auto mockPoller = std::make_unique<MockPoller>();
        auto mockPollerFactory = std::make_unique<MockPollerFactory>();
        EXPECT_CALL(*mockPollerFactory, create).WillOnce(Return(ByMove(std::move(mockPoller))));
        Common::ZeroMQWrapper::replacePollerFactory(std::move(mockPollerFactory));

        auto mockSocketReplier = std::make_unique<MockSocketReplier>();
        auto mockZmqContext = std::make_shared<MockZmqContext>();
        EXPECT_CALL(*mockZmqContext, getReplier).WillOnce(Return(ByMove(std::move(mockSocketReplier))));
        auto pluginManager = std::make_shared<PluginManager>(mockZmqContext);
        pluginManager->setEventReceiver(eventReceiver);

        taskProcessor->start();

        return { pluginManager, eventReceiver, taskProcessor };
    }

    // Tests for queuing CORE actions using queueAction when PluginManager is in outbreak mode
    class TestPluginManagerQueueActionCoreInOutbreakMode : public TestPluginManagerQueueAction
    {
    };
} // namespace

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, ActionFileMissingLogsWarningAndDoesNotResetOutbreakMode)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Throw(IFileNotFoundException("not found")));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_TRUE(appenderContains("WARN - Unable to read Health Action Task at: "
                                 "/opt/sophos-spl/base/mcs/action/test_action_one.xml due to: not found"));
    EXPECT_TRUE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, ActionFileMissingDoesNotPreventOutbreakModeFromBeingResetLater)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Throw(IFileNotFoundException("not found")));
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_two.xml"))
        .WillRepeatedly(Return(
            R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)"));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    ASSERT_TRUE(eventReceiver->outbreakMode());
    pluginManager->queueAction("CORE", "test_action_two.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_FALSE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, EmptyActionFileLogsAtDebugAndDoesNotResetOutbreakMode)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Return(""));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_TRUE(appenderContains("DEBUG - Ignoring action as it is an empty XML"));
    EXPECT_TRUE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, EmptyActionFileDoesNotPreventOutbreakModeFromBeingResetLater)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Return(""));
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_two.xml"))
        .WillRepeatedly(Return(
            R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)"));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    ASSERT_TRUE(eventReceiver->outbreakMode());
    pluginManager->queueAction("CORE", "test_action_two.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_FALSE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, InvalidXmlLogsErrorAndDoesNotLeaveOutbreakMode)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Return("{}"));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_TRUE(
        appenderContains("ERROR - Failed to parse action XML: {}: Error parsing xml: not well-formed (invalid token)"));
    EXPECT_TRUE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, InvalidXmlDoesNotPreventOutbreakModeFromBeingResetLater)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Return("{}"));
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_two.xml"))
        .WillRepeatedly(Return(
            R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)"));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    ASSERT_TRUE(eventReceiver->outbreakMode());
    pluginManager->queueAction("CORE", "test_action_two.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_FALSE(eventReceiver->outbreakMode());
}

TEST_F(
    TestPluginManagerQueueActionCoreInOutbreakMode,
    ActionFileWithMissingPermissionsLogsWarningAndDoesNotResetOutbreakMode)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Throw(IPermissionDeniedException("lacking permissions")));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_TRUE(appenderContains("WARN - Unable to read Health Action Task at: "
                                 "/opt/sophos-spl/base/mcs/action/test_action_one.xml due to: lacking permissions"));
    EXPECT_TRUE(eventReceiver->outbreakMode());
}

TEST_F(
    TestPluginManagerQueueActionCoreInOutbreakMode,
    ActionFileWithMissingPermissionsDoesNotPreventOutbreakModeFromBeingResetLater)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Throw(IPermissionDeniedException("lacking permissions")));
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_two.xml"))
        .WillRepeatedly(Return(
            R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)"));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    ASSERT_TRUE(eventReceiver->outbreakMode());
    pluginManager->queueAction("CORE", "test_action_two.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_FALSE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, TooLargeActionFileLogsWarningAndDoesNotResetOutbreakMode)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Throw(IFileTooLargeException("too large")));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_TRUE(appenderContains("WARN - Unable to read Health Action Task at: "
                                 "/opt/sophos-spl/base/mcs/action/test_action_one.xml due to: too large"));
    EXPECT_TRUE(eventReceiver->outbreakMode());
}

TEST_F(TestPluginManagerQueueActionCoreInOutbreakMode, TooLargeActionFileDoesNotPreventOutbreakModeFromBeingResetLater)
{
    // Arrange
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_one.xml"))
        .WillRepeatedly(Throw(IFileTooLargeException("too large")));
    EXPECT_CALL(*mockFileSystem_, readFile("/opt/sophos-spl/base/mcs/action/test_action_two.xml"))
        .WillRepeatedly(Return(
            R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)"));
    Tests::replaceFileSystem(std::move(mockFileSystem_));
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    auto [pluginManager, eventReceiver, taskProcessor] = createPluginManager();
    enterOutbreakMode(*eventReceiver);

    // Act
    pluginManager->queueAction("CORE", "test_action_one.xml", "");
    ASSERT_TRUE(eventReceiver->outbreakMode());
    pluginManager->queueAction("CORE", "test_action_two.xml", "");
    taskProcessor->stop();

    // Assert
    EXPECT_FALSE(eventReceiver->outbreakMode());
}
