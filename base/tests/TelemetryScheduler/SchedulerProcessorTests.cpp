/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryExeConfigImpl/Constants.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using TelemetrySchedulerImpl::PluginCallback;
using TelemetrySchedulerImpl::SchedulerProcessor;
using TelemetrySchedulerImpl::Task;
using TelemetrySchedulerImpl::TaskQueue;

class SchedulerProcessorTests : public ::testing::Test
{
public:
    MockFileSystem* m_mockFileSystem = nullptr;
    MockFilePermissions* m_mockFilePermissions = nullptr;
    MockedApplicationPathManager m_mockPathManager;

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
    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));

    queue->pushPriority(Task::Shutdown);
    processor.run();
}

TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin) // NOLINT
{
    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);
    PluginCallback pluginCallback(queue);

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));

    pluginCallback.onShutdown();
    processor.run();
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ValidStatusFileWithScheduleInPast) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    auto task = queue->pop();
    EXPECT_EQ(task, Task::RunTelemetry);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ValidStatusFileWithScheduleInFuture) // NOLINT
{
    auto future = std::chrono::system_clock::now() + std::chrono::seconds(2);;
    size_t futureInSecondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(future.time_since_epoch()).count();
    std::stringstream statusFileContents;
    statusFileContents << R"({"scheduled-time":)" << futureInSecondsSinceEpoch << "}";

    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(statusFileContents.str()));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":"invalid"})"));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetryStatusFilePath));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ErrorReadingStatusFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileCannotRewrite) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndMissingSupplementaryFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(false));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileCannotReadSupplementaryFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndSupplementaryFileWithMissingInterval) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"no-interval":86400})"));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndInvalidSupplementaryFile) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":"86400"})"));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFileCannotRemove) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryStatusFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":"invalid"})"));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetryStatusFilePath))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);

    queue->pushPriority(Task::Shutdown); // schedule telemetry then stop
    processor.run();

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}
