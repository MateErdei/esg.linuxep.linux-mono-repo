/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DerivedSchedulerProcessor.h"

#include <TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::PluginCallback;
using TelemetrySchedulerImpl::SchedulerProcessor;
using TelemetrySchedulerImpl::Task;
using TelemetrySchedulerImpl::TaskQueue;

class SchedulerProcessorTests : public ::testing::Test
{
public:
    MockFileSystem* m_mockFileSystem = nullptr;
    MockFilePermissions* m_mockFilePermissions = nullptr;
    MockedApplicationPathManager  m_mockPathManager;

    const std::string m_supplementaryConfigFilePath = "/opt/sophos-spl/base/etc/telemetry-config.json";
    const std::string m_telemetryExeConfigFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry-exe.json";
    const std::string m_telemetryStatusFilePath = "/opt/sophos-spl/base/telemetry/var/tscheduler-status.json";

    void SetUp() override
    {
        std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
        m_mockFileSystem = mockfileSystem.get();
        Tests::replaceFileSystem(std::move(mockfileSystem));

        std::unique_ptr<MockFilePermissions> mockfilePermissions(new StrictMock<MockFilePermissions>());
        m_mockFilePermissions = mockfilePermissions.get();
        Tests::replaceFilePermissions(std::move(mockfilePermissions));
    }

    void TearDown() override {}
};

 TEST_F(SchedulerProcessorTests, CanBeConstructed) // NOLINT
{
    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);
}

 TEST_F(SchedulerProcessorTests, ConstructionWithNullQueue) // NOLINT
{
    std::shared_ptr<TaskQueue> queue;

    EXPECT_THROW(SchedulerProcessor processor(queue, m_mockPathManager), std::invalid_argument); // NOLINT
}

 TEST_F(SchedulerProcessorTests, CanBeStopped) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(10);

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);
    std::atomic<bool> done(false);

    std::thread processorThread([&] {
        processor.run();
        done = true;
    });

    EXPECT_FALSE(done);

    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_FALSE(done);

    queue->pushPriority(Task::Shutdown);
    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_TRUE(done);

    processorThread.join();
}

 TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(10);

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);
    PluginCallback pluginCallback(queue);
    std::atomic<bool> done(false);

    std::thread processorThread([&] {
        processor.run();
        done = true;
    });

    EXPECT_FALSE(done);

    pluginCallback.onShutdown();
    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_TRUE(done);

    processorThread.join();
}

// TODO: it might be better to test the following cases via run() or waitToRunTelemetry()!

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ValidStatusFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath()).WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry();

    // TODO: LINUXEP-6639 add check that RunTelemetry is queued
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ErrorReadingStatusFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, getScheduledTimeUsingSupplementaryFile_ValidSupplementaryFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, getScheduledTimeUsingSupplementaryFile_InvalidSupplementaryFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, getScheduledTimeUsingSupplementaryFile_ZeroInterval) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, getIntervalFromSupplementaryFile_ValidSupplementaryFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, getIntervalFromSupplementaryFile_InvalidSupplementaryFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, getIntervalFromSupplementaryFile_MissingSupplementaryFile) // NOLINT
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);;
}
