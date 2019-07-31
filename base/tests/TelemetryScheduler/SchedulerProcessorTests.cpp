/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DerivedSchedulerProcessor.h"
#include "MockTaskQueue.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Process/IProcessException.h>
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
using TelemetrySchedulerImpl::SchedulerTask;
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
    const std::string m_telemetryExecutableFilePath = "/opt/sophos-spl/base/bin/telemetry";

    const std::string m_validTelemetryConfigJson = R"(
    {
        "server": "localhost",
        "port": 443,
        "verb": "PUT",
        "additionalHeaders": ["x-amz-acl:bucket-owner-full-control"],
        "resourceRoot": "linux/dev",
        "interval": 3600
    })";

    const std::string m_invalidTelemetryConfigJson = R"(
    {
        "server": "localhost",
        "port": 443,
        "verb": "PUT",
        "additionalHeaders": ["x-amz-acl:bucket-owner-full-control"],
        "resourceRoot": "linux/dev",
        "interval": 3600
    )"; // missing closing brace

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
    const std::chrono::milliseconds delay(1);

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);
    std::atomic<int> stage = 0;

    std::thread processorThread([&] {
        stage = 1;
        processor.run();
        stage = 2;
    });

    for (int i = 0; i < 300; i++)
    {
        if (stage == 1)
            break;
        std::this_thread::sleep_for(delay); // attempt to allow processor to run
    }

    EXPECT_EQ(stage, 1);

    queue->pushPriority(SchedulerTask::Shutdown);

    for (int i = 0; i < 300; i++)
    {
        if (stage == 2)
            break;
        std::this_thread::sleep_for(delay); // attempt to allow processor to run
    }

    EXPECT_EQ(stage, 2);

    processorThread.join();
}

TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin) // NOLINT
{
    // const std::chrono::milliseconds delay(10);

    auto mockQueue = std::make_shared<StrictMock<MockTaskQueue>>();
    SchedulerProcessor processor(mockQueue, m_mockPathManager);
    PluginCallback pluginCallback(mockQueue);
    std::atomic<bool> done(false);

    EXPECT_CALL(*mockQueue, pushPriority(TelemetrySchedulerImpl::SchedulerTask::Shutdown));
    EXPECT_CALL(*mockQueue, pop()).WillOnce(Return(TelemetrySchedulerImpl::SchedulerTask::Shutdown));

    pluginCallback.onShutdown();

    std::thread processorThread([&] {
        processor.run();
        done = true;
    });

    processorThread.join();

    EXPECT_TRUE(done);
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InitWithValidStatusFileWithScheduleInPast) // NOLINT
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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    auto task = queue->pop();
    EXPECT_EQ(task, SchedulerTask::RunTelemetry);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_RescheduleWithValidStatusFileWithScheduleInPast) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetryStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_supplementaryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":1})"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(false);

    auto task = queue->pop();
    EXPECT_EQ(task, SchedulerTask::RunTelemetry);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InitWithValidStatusFileWithScheduleInFuture) // NOLINT
{
    auto inTheFuture = std::chrono::system_clock::now() + std::chrono::seconds(2);
    size_t inTheFutureInSecondsSinceEpoch =
        std::chrono::duration_cast<std::chrono::seconds>(inTheFuture.time_since_epoch()).count();
    std::stringstream statusFileContents;
    statusFileContents << R"({"scheduled-time":)" << inTheFutureInSecondsSinceEpoch << "}";

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

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
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, runningTelemetryExecutableIsSuccessfull) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));

    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{m_telemetryExeConfigFilePath}));
    EXPECT_CALL(*m_mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*m_mockProcess, exitCode()).WillOnce(Return(0));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::CheckExecutableFinished);

    processor.checkExecutableFinished();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}

TEST_F(SchedulerProcessorTests, runningTelemetryExecutableTimesOut) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));

    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{m_telemetryExeConfigFilePath}));
    EXPECT_CALL(*m_mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
    EXPECT_CALL(*m_mockProcess, kill());

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::CheckExecutableFinished);

    processor.checkExecutableFinished();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}

TEST_F(SchedulerProcessorTests, runningTelemetryExecutableGivesFailure) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));

    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{m_telemetryExeConfigFilePath}));
    EXPECT_CALL(*m_mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*m_mockProcess, exitCode()).WillOnce(Return(1));
    EXPECT_CALL(*m_mockProcess, output()).WillOnce(Return("stdout and stderr output"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::CheckExecutableFinished);

    processor.checkExecutableFinished();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}

TEST_F(SchedulerProcessorTests, runTelemetry_ErrorReadingConfig) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}

TEST_F(SchedulerProcessorTests, runTelemetry_ConfigHasInvalidJson) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_invalidTelemetryConfigJson));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}

TEST_F(SchedulerProcessorTests, runTelemetry_ErrorWritingExeConfig) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(
        *m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}

TEST_F(SchedulerProcessorTests, runTelemetry_ErrorAttemptingToRunExe) // NOLINT
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_supplementaryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(m_supplementaryConfigFilePath, Common::TelemetryExeConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));

    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }))
        .WillOnce(Throw(Common::Process::IProcessException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask::WaitToRunTelemetry);
}
