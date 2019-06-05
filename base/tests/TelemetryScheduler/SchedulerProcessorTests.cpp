/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockTaskQueue.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/TelemetryExeConfigImpl/Constants.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::PluginCallback;
using TelemetrySchedulerImpl::SchedulerProcessor;
using TelemetrySchedulerImpl::Task;
using TelemetrySchedulerImpl::TaskQueue;

using ::testing::DefaultValue;

class SchedulerProcessorTests : public ::testing::Test
{
public:
    MockFileSystem* m_mockFileSystem = nullptr;
    MockFilePermissions* m_mockFilePermissions = nullptr;
    MockedApplicationPathManager m_mockPathManager;
    std::unique_ptr<MockProcess> m_mockProcess;
    std::shared_ptr<MockTaskQueue> m_mockQueue;

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

        setupMockProcess();
        m_mockQueue = std::make_shared<StrictMock<MockTaskQueue>>();

        // ON_CALL(*m_mockQueue, pop()).WillByDefault(Return(TelemetrySchedulerImpl::Task::WaitToRunTelemetry));
    }

    void setupMockProcess()
    {
        m_mockProcess.reset(new StrictMock<MockProcess>());
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([this]() { return std::move(m_mockProcess); });
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
    //    const std::chrono::milliseconds delay(10);
    //
    //    auto mockQueue = std::make_shared<MockTaskQueue>();
    //    SchedulerProcessor processor(mockQueue, m_mockPathManager);
    //    std::atomic<bool> done(false);
    //
    //    std::thread processorThread([&] {
    //      processor.run();
    //      done = true;
    //    });
    //
    //    EXPECT_FALSE(done);
    //
    //    std::this_thread::sleep_for(delay); // attempt to allow processor to run
    //
    //    EXPECT_FALSE(done);
    //
    //    mockQueue->pushPriority(Task::Shutdown);
    //    std::this_thread::sleep_for(delay); // attempt to allow processor to run
    //
    //    EXPECT_TRUE(done);
    //
    //    processorThread.join();
}

TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin) // NOLINT
{
    // const std::chrono::milliseconds delay(10);

    auto mockQueue = std::make_shared<StrictMock<MockTaskQueue>>();
    SchedulerProcessor processor(mockQueue, m_mockPathManager);
    PluginCallback pluginCallback(mockQueue);
    std::atomic<bool> done(false);

    EXPECT_CALL(*mockQueue, pushPriority(TelemetrySchedulerImpl::Task::Shutdown));
    EXPECT_CALL(*mockQueue, pop()).WillOnce(Return(TelemetrySchedulerImpl::Task::Shutdown));

    pluginCallback.onShutdown();

    std::thread processorThread([&] {
      processor.run();
      done = true;
    });

    processorThread.join();
    // std::this_thread::sleep_for(delay); // attempt to allow processor to run

    EXPECT_TRUE(done);
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ValidStatusFileWithScheduleInPast) // NOLINT
{
    auto mockQueue = std::make_shared<StrictMock<MockTaskQueue>>();
    SchedulerProcessor processor(mockQueue, m_mockPathManager);
    {
        InSequence inseq1, inseq2;
        EXPECT_CALL(*mockQueue, pop()).WillOnce(Return(TelemetrySchedulerImpl::Task::WaitToRunTelemetry));
        EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillOnce(Return(m_telemetryStatusFilePath));
        EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
        EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillOnce(Return(m_telemetryStatusFilePath));
        EXPECT_CALL(
            *m_mockFileSystem,
            readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(R"({"scheduled-time":1559556823})"));

        EXPECT_CALL(*mockQueue, push(TelemetrySchedulerImpl::Task::RunTelemetry));

        // return shutdown to stop test
        EXPECT_CALL(*mockQueue, pop()).WillOnce(Return(TelemetrySchedulerImpl::Task::Shutdown));
    }

    processor.run();
}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ValidStatusFileWithScheduleInFuture) // NOLINT
//{
//    auto future = std::chrono::system_clock::now() + std::chrono::seconds(2);
//    size_t futureInSecondsSinceEpoch =
//        std::chrono::duration_cast<std::chrono::seconds>(future.time_since_epoch()).count();
//    std::stringstream statusFileContents;
//    statusFileContents << R"({"scheduled-time":)" << futureInSecondsSinceEpoch << "}";
//
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(
//            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
//            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(statusFileContents.str()));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_TRUE(processor.delayingTelemetryRun());
//    EXPECT_FALSE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFile) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem, readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"scheduled-time":"invalid"})"));
//    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetryStatusFilePath));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"interval":86400})"));
//    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_TRUE(processor.delayingTelemetryRun());
//    EXPECT_FALSE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFile) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"interval":86400})"));
//    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_TRUE(processor.delayingTelemetryRun());
//    EXPECT_FALSE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ErrorReadingStatusFile) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem, readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"interval":86400})"));
//    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_TRUE(processor.delayingTelemetryRun());
//    EXPECT_FALSE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileCannotRewrite) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"interval":86400})"));
//    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _))
//        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_TRUE(processor.delayingTelemetryRun());
//    EXPECT_FALSE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndMissingSupplementaryFile) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(false));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_FALSE(processor.delayingTelemetryRun());
//    EXPECT_TRUE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileCannotReadSupplementaryFile) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_FALSE(processor.delayingTelemetryRun());
//    EXPECT_TRUE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndSupplementaryFileWithMissingInterval) //
// NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"no-interval":86400})"));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_FALSE(processor.delayingTelemetryRun());
//    EXPECT_TRUE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndInvalidSupplementaryFile) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"interval":"86400"})"));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_FALSE(processor.delayingTelemetryRun());
//    EXPECT_TRUE(processor.delayingConfigurationCheck());
//}
//
// TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFileCannotRemove) // NOLINT
//{
//    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
//        .WillRepeatedly(Return(m_telemetryStatusFilePath));
//    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
//        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem, readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"scheduled-time":"invalid"})"));
//    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetryStatusFilePath))
//        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
//
//    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
//    EXPECT_CALL(
//        *m_mockFileSystem,
//        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
//        .WillOnce(Return(R"({"interval":86400})"));
//    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, m_mockPathManager);
//
//    processor.waitToRunTelemetry();
//
//    EXPECT_TRUE(processor.delayingTelemetryRun());
//    EXPECT_FALSE(processor.delayingConfigurationCheck());
//}
