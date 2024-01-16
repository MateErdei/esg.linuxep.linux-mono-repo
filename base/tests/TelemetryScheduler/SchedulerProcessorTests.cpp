// Copyright 2019-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "DerivedSchedulerProcessor.h"
#include "MockTaskQueue.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Process/IProcessException.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/TelemetryConfigImpl/Constants.h"
#include "TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h"
#include "TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/UtilityImpl/TestStringGenerator.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::PluginCallback;
using TelemetrySchedulerImpl::SchedulerProcessor;
using TelemetrySchedulerImpl::SchedulerTask;
using TelemetrySchedulerImpl::TaskQueue;

using ::testing::DefaultValue;

class SchedulerProcessorTests : public MemoryAppenderUsingTests
{
public:
    SchedulerProcessorTests() : MemoryAppenderUsingTests("telemetryScheduler"), usingMemoryAppender{ *this } {}

    MockFileSystem* m_mockFileSystem = nullptr;
    MockFilePermissions* m_mockFilePermissions = nullptr;
    MockedApplicationPathManager m_mockPathManager;
    std::unique_ptr<MockProcess> m_mockProcess;
    std::shared_ptr<MockTaskQueue> m_mockQueue;
    UsingMemoryAppender usingMemoryAppender;

    const std::string m_telemetryConfigFilePath = "/opt/sophos-spl/base/etc/telemetry-config.json";
    const std::string m_telemetryExeConfigFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry-exe.json";
    const std::string m_telemetrySchedulerStatusFilePath = "/opt/sophos-spl/base/telemetry/var/tscheduler-status.json";
    const std::string m_telemetryExecutableFilePath = "/opt/sophos-spl/base/bin/telemetry";

    const std::string m_validTelemetryConfigJson = R"(
    {
        "server": "localhost",
        "port": 443,
        "verb": "PUT",
        "additionalHeaders": {"x-amz-acl":"bucket-owner-full-control"},
        "resourceRoot": "linux/dev",
        "interval": 1
    })";

    const std::string m_invalidTelemetryConfigJson = R"(
    {
        "server": "localhost",
        "port": 443,
        "verb": "PUT",
        "additionalHeaders": {"x-amz-acl":"bucket-owner-full-control"},
        "resourceRoot": "linux/dev",
        "interval": "1"
    })"; // interval as string

    const std::string m_invalidTelemetryConfigBigIntervalJson = R"(
    {
        "server": "localhost",
        "port": 443,
        "verb": "PUT",
        "additionalHeaders": {"x-amz-acl":"bucket-owner-full-control"},
        "resourceRoot": "linux/dev",
        "interval": 12000000000000000
    })"; // interval as big number

    const std::string m_invalidTelemetryConfigNegativeIntervalJson = R"(
    {
        "server": "localhost",
        "port": 443,
        "verb": "PUT",
        "additionalHeaders": {"x-amz-acl":"bucket-owner-full-control"},
        "resourceRoot": "linux/dev",
        "interval": -1
    })"; // interval negative

    const std::string m_validALCPolicy = R"sophos(<?xml version="1.0"?>
        <AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
          <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
        <AUConfig platform="Linux">
        <primary_location>
          <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
        </primary_location>
        <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>"
        <cloud_subscriptions>"
            <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>"
        </cloud_subscriptions>"
        </AUConfig>
        <server_names>
          <telemetry>test.sophosupd.com</telemetry>
        </server_names>
        <Features>
            <Feature id="CORE"/>
            <Feature id="SDU"/>
        </Features>
        </AUConfigurations>
    )sophos";

    void SetUp() override
    {
        m_mockFileSystem = new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystem));


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
    Tests::ScopedReplaceFileSystem m_replacer;

    static std::string generateAlcPolicyWithTelemetryHostname(const std::string& hostname)
    {
        return std::string(R"sophos(<?xml version="1.0"?>
            <AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
              <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
            <AUConfig platform="Linux">
            <primary_location>
              <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
            </primary_location>
            <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>"
            <cloud_subscriptions>"
                <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>"
            </cloud_subscriptions>"
            </AUConfig>
            <server_names>
                <telemetry>)sophos") + hostname + R"sophos(</telemetry>
            </server_names>
            <Features>
                <Feature id="CORE"/>
                <Feature id="SDU"/>
            </Features>
            </AUConfigurations>
        )sophos";
    }
};

TEST_F(SchedulerProcessorTests, CanBeConstructed)
{
    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, m_mockPathManager);
}

TEST_F(SchedulerProcessorTests, ConstructionWithNullQueue)
{
    std::shared_ptr<TaskQueue> queue;

    EXPECT_THROW(SchedulerProcessor processor(queue, m_mockPathManager), std::invalid_argument);
}

TEST_F(SchedulerProcessorTests, CanBeStopped)
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

    queue->pushPriority({SchedulerTask::TaskType::Shutdown});

    for (int i = 0; i < 300; i++)
    {
        if (stage == 2)
            break;
        std::this_thread::sleep_for(delay); // attempt to allow processor to run
    }

    EXPECT_EQ(stage, 2);

    processorThread.join();
}

TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin)
{
    // const std::chrono::milliseconds delay(10);

    auto mockQueue = std::make_shared<StrictMock<MockTaskQueue>>();
    SchedulerProcessor processor(mockQueue, m_mockPathManager);
    PluginCallback pluginCallback(mockQueue);
    std::atomic<bool> done(false);

    EXPECT_CALL(*mockQueue, pushPriority(SchedulerTask{TelemetrySchedulerImpl::SchedulerTask::TaskType::Shutdown}));
    EXPECT_CALL(*mockQueue, pop()).WillOnce(Return(SchedulerTask{TelemetrySchedulerImpl::SchedulerTask::TaskType::Shutdown}));

    pluginCallback.onShutdown();

    std::thread processorThread([&] {
        processor.run();
        done = true;
    });

    processorThread.join();

    EXPECT_TRUE(done);
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InitWithValidStatusFileWithScheduleInPast)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    auto task = queue->pop();
    EXPECT_EQ(task, SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry}); // Task from processALCPolicy()
    task = queue->pop();
    EXPECT_EQ(task, SchedulerTask{SchedulerTask::TaskType::RunTelemetry}); // Task from waitToRunTelemetry()

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_TelemetryDisabledViaConfig)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":0})"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_TelemetryDisabledViaStatus)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":0})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":0})"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_RescheduleWithValidStatusFileWithScheduleInPast)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(false);

    auto task = queue->pop();
    EXPECT_EQ(task, SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});
    task = queue->pop();
    EXPECT_EQ(task, SchedulerTask{SchedulerTask::TaskType::RunTelemetry});

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InitWithValidStatusFileWithScheduleInFuture)
{
    /*
     * Upgrade case - scheduled time is in the future at start time
     */
    auto inTheFuture = std::chrono::system_clock::now() + std::chrono::seconds(2);
    size_t inTheFutureInSecondsSinceEpoch =
        std::chrono::duration_cast<std::chrono::seconds>(inTheFuture.time_since_epoch()).count();
    std::stringstream schedulerStatusFileContents;
    schedulerStatusFileContents << R"({"scheduled-time":)" << inTheFutureInSecondsSinceEpoch << "}";

    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(schedulerStatusFileContents.str()));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true); // Start up

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFile)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetrySchedulerStatusFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":"invalid"})"));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFile)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ErrorReadingStatusFile)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetrySchedulerStatusFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileCannotRewrite)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndMissingConfigFile)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(false));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileCannotReadConfigFile)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndConfigFileWithMissingInterval)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"no-interval":1})"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndInvalidConfigFile)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
            .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(m_invalidTelemetryConfigJson));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndInvalidConfigFileBigNumberForInterval)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
            .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(m_invalidTelemetryConfigBigIntervalJson));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}


TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFileAndInvalidConfigFileNegativeNumberForInterval)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
            .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(m_invalidTelemetryConfigNegativeIntervalJson));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_FALSE(processor.delayingTelemetryRun());
    EXPECT_TRUE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFileCannotRemove)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetrySchedulerStatusFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":"invalid"})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetrySchedulerStatusFilePath))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFileBigNumber)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
            .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetrySchedulerStatusFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(R"({"scheduled-time": 12000000000000000})"));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetrySchedulerStatusFilePath))
            .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFileNegativeNumber)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
            .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
            .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetrySchedulerStatusFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(R"({"scheduled-time": -1})"));
    EXPECT_CALL(
            *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
            .WillOnce(Return(R"({"interval":86400})"));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_telemetrySchedulerStatusFilePath))
            .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager);

    processor.processALCPolicy(m_validALCPolicy);
    processor.waitToRunTelemetry(true);

    EXPECT_TRUE(processor.delayingTelemetryRun());
    EXPECT_FALSE(processor.delayingConfigurationCheck());
}

TEST_F(SchedulerProcessorTests, runningTelemetryExecutableIsSuccessful)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return("machineId"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));
    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }));
    EXPECT_CALL(*m_mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*m_mockProcess, exitCode()).WillOnce(Return(0));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.processALCPolicy(m_validALCPolicy);
    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});
    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::CheckExecutableFinished});

    processor.checkExecutableFinished();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runningTelemetryWithTelemetryDisabledViaConfig)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"interval":0})"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runningTelemetryWithTelemetryDisabledViaStatus)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":0})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runningTelemetryExecutableTimesOut)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
                m_telemetrySchedulerStatusFilePath,
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return("machineId"));
    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }));
    EXPECT_CALL(*m_mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
    EXPECT_CALL(*m_mockProcess, kill());

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.processALCPolicy(m_validALCPolicy);

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::CheckExecutableFinished});

    processor.checkExecutableFinished();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runningTelemetryExecutableGivesFailure)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return("machineId"));
    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }));
    EXPECT_CALL(*m_mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*m_mockProcess, exitCode()).WillOnce(Return(1));
    EXPECT_CALL(*m_mockProcess, output()).WillOnce(Return("stdout and stderr output"));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.processALCPolicy(m_validALCPolicy);

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::CheckExecutableFinished});

    processor.checkExecutableFinished();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runTelemetry_ErrorReadingConfig)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runTelemetry_ConfigHasInvalidJson)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_invalidTelemetryConfigJson));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runTelemetry_NoMachineId)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return(""));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.processALCPolicy(m_validALCPolicy);
    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});
    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runTelemetry_ErrorWritingExeConfig)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillOnce(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return("machineId"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.processALCPolicy(m_validALCPolicy);
    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});
    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, runTelemetry_ErrorAttemptingToRunExe)
{
    EXPECT_CALL(m_mockPathManager, getTelemetrySchedulerStatusFilePath())
        .WillRepeatedly(Return(m_telemetrySchedulerStatusFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetrySupplementaryFilePath())
        .WillRepeatedly(Return(m_telemetryConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExeConfigFilePath())
        .WillRepeatedly(Return(m_telemetryExeConfigFilePath));
    EXPECT_CALL(m_mockPathManager, getTelemetryExecutableFilePath())
        .WillRepeatedly(Return(m_telemetryExecutableFilePath));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_mockPathManager.getTelemetrySchedulerStatusFilePath(),
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillOnce(Return("machineId"));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _));
    EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }))
        .WillOnce(Throw(Common::Process::IProcessException("error")));

    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, m_mockPathManager, 0s, 0s);

    processor.processALCPolicy(m_validALCPolicy);
    processor.runTelemetry();

    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::InitialWaitToRunTelemetry});
    EXPECT_EQ(queue->pop(), SchedulerTask{SchedulerTask::TaskType::WaitToRunTelemetry});
}

TEST_F(SchedulerProcessorTests, calculateScheduledTimeNewInstall)
{
    bool newInstall = true;
    bool runScheduledInPastNow = true; // fresh start
    std::optional<TelemetrySchedulerImpl::SchedulerStatus> status; // no status file
    unsigned int interval = 1000;
    time_point timePointNow = system_clock::now();
    time_point scheduledTime = SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(status,
                                                                                           newInstall,
                                                                                           interval,
                                                                                           runScheduledInPastNow,
                                                                                           timePointNow);
    EXPECT_EQ(scheduledTime, timePointNow + minutes{10});
}

/**
 * Normal upgrade case - status file exists and contains scheduled time in the future
 */
TEST_F(SchedulerProcessorTests, calculateScheduledTimeNormalUpgrade)
{
    bool newInstall = false; // previous status file
    bool runScheduledInPastNow = true; // just started
    unsigned int interval = 1000;

    time_point timePointNow = system_clock::now();
    time_point previousScheduledTime = timePointNow + seconds{900};

    TelemetrySchedulerImpl::SchedulerStatus status;
    status.setTelemetryScheduledTime(previousScheduledTime);

    time_point scheduledTime = SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(status,
                                                       newInstall,
                                                       interval, runScheduledInPastNow, timePointNow);

    // Should return previous scheduled time
    EXPECT_EQ(scheduledTime, previousScheduledTime);
}

/**
 * Upgrade where we are over due sending telemetry
 * Status file contains scheduled time in the past
 */
TEST_F(SchedulerProcessorTests, calculateScheduledTimeUpgradeOverdueSendingTelemetry)
{
    bool newInstall = false;
    bool runScheduledInPastNow = true;
    unsigned int interval = 1000;

    time_point timePointNow = system_clock::now();
    time_point previousScheduledTime = timePointNow - seconds{500};

    TelemetrySchedulerImpl::SchedulerStatus status;
    status.setTelemetryScheduledTime(previousScheduledTime);

    time_point scheduledTime = SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(status,
                                                       newInstall,
                                                       interval, runScheduledInPastNow, timePointNow);

    EXPECT_EQ(scheduledTime, previousScheduledTime);
}

TEST_F(SchedulerProcessorTests, calculateScheduledTimeNormalOperation)
{
    bool newInstall = false;
    bool runScheduledInPastNow = false;
    unsigned int interval = 1000;

    time_point timePointNow = system_clock::now();
    time_point previousScheduledTime = timePointNow + seconds{900};
    time_point previousStartTime = timePointNow - seconds{100};

    TelemetrySchedulerImpl::SchedulerStatus status;
    status.setTelemetryScheduledTime(previousScheduledTime);
    status.setLastTelemetryStartTime(previousStartTime);

    time_point scheduledTime = SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(status,
                                                                                           newInstall,
                                                                                           interval, runScheduledInPastNow, timePointNow);
    // Calulate next time based on when we last ran telemetry
    EXPECT_EQ(scheduledTime, previousStartTime + seconds{interval});
}

TEST_F(SchedulerProcessorTests, calculateScheduledTimeStatusFileCorrupt)
{
    bool newInstall = false; // status file was present but corrupt
    bool runScheduledInPastNow = false;
    unsigned int interval = 1000;
    time_point timePointNow = system_clock::now();
    std::optional<TelemetrySchedulerImpl::SchedulerStatus> status; // no status file

    time_point scheduledTime = SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(status,
                                                                                           newInstall,
                                                                                           interval,
                                                                                           runScheduledInPastNow,
                                                                                           timePointNow);
    // Calculate next time based on now, since status file is corrupt
    EXPECT_EQ(scheduledTime, timePointNow + seconds{interval});
}

TEST_F(SchedulerProcessorTests, calculateScheduledTimeUpgradeNotNewStartLastScheduledTimeInPast)
{
    bool newInstall = false;
    bool runScheduledInPastNow = false;
    unsigned int interval = 1000;

    time_point timePointNow = system_clock::now();
    time_point previousScheduledTime = timePointNow - seconds{500};

    TelemetrySchedulerImpl::SchedulerStatus status;
    status.setTelemetryScheduledTime(previousScheduledTime);

    time_point scheduledTime = SchedulerProcessor::calculateScheduledTimeFromtheStatusFile(status,
                                                                                           newInstall,
                                                                                           interval,
                                                                                           runScheduledInPastNow,
                                                                                           timePointNow);

    EXPECT_EQ(scheduledTime, previousScheduledTime + seconds{interval});
}


// Tests for checking ALC Policy with different Telemetry field(s) and value(s).

class SchedulerProcessorValidHostnameTests : public SchedulerProcessorTests, public testing::WithParamInterface<std::string>
{
    using SchedulerProcessorTests::SchedulerProcessorTests;
};

TEST_P(SchedulerProcessorValidHostnameTests, AfterRecevingAlcPolicyExeConfigIsWrittenWithCorrectServerAndTelemetryExecutableIsRun)
{
    const auto policy = generateAlcPolicyWithTelemetryHostname(GetParam());

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_telemetrySchedulerStatusFilePath,
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _)).WillRepeatedly(Return());
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return("machineId"));

    auto queue = std::make_shared<TaskQueue>();

    // add policy task
    // it should write config
    // wait for telemetry executable to run
    // send the shutdown task

    {
        InSequence seq;

        EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _))
            .WillOnce(Invoke(
                [](const std::string&, const std::string& content)
                {
                    auto j = nlohmann::json::parse(content);
                    EXPECT_EQ(j["server"], GetParam());
                }));

        EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
        EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }));

        EXPECT_CALL(*m_mockProcess, getStatus())
            .WillOnce(InvokeWithoutArgs(
                [&queue]()
                {
                    queue->push({ SchedulerTask::TaskType::Shutdown });
                    return Common::Process::ProcessStatus::FINISHED;
                }));
        EXPECT_CALL(*m_mockProcess, exitCode()).WillOnce(Return(0));
    }

    auto pathManager = std::make_shared<Common::ApplicationConfigurationImpl::ApplicationPathManager>();
    DerivedSchedulerProcessor processor(queue, *pathManager, 0s, 0s);

    std::thread processorThread([&] {
        processor.run();
    });

    queue->push({ .taskType = SchedulerTask::TaskType::Policy, .content = policy, .appId = "ALC" });
    processorThread.join();
}

INSTANTIATE_TEST_SUITE_P(
    SchedulerProcessorValidHostnameTestsInstantiation,
    SchedulerProcessorValidHostnameTests,
    testing::Values(
        "sus.sophosupd.com",
        "sdds3.sophosupd.com",
        "sdds3.sophosupd.net",
        "t1.sophosupd.com",
        "test.sophosupd.com"));

TEST_F(SchedulerProcessorTests, AfterRecevingAlcPolicyWithEmptyTelemetryHostnameExeConfigIsNotWrittenAndTelemetryExecutableIsNotRun)
{
    const auto policy = generateAlcPolicyWithTelemetryHostname("");

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_telemetrySchedulerStatusFilePath,
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return("machineId"));

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _)).Times(0);
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath })).Times(0);

    auto queue = std::make_shared<TaskQueue>();
    auto pathManager = std::make_shared<Common::ApplicationConfigurationImpl::ApplicationPathManager>();
    DerivedSchedulerProcessor processor(queue, *pathManager, 0s, 0s);

    std::thread processorThread([&] {
                                    processor.run();
                                });

    queue->push({ .taskType = SchedulerTask::TaskType::Policy, .content = policy, .appId = "ALC" });

    // Need to wait for the log to know that we can push the shutdown task, as that means the scheduler decided not to run it
    EXPECT_TRUE(waitForLog("Telemetry reporting is currently disabled - will check again in 0 seconds"));
    queue->push({ SchedulerTask::TaskType::Shutdown });

    processorThread.join();
}

TEST_F(SchedulerProcessorTests, AfterRecevingAlcPolicyWithNoTelemetryFieldExeConfigIsWrittenWithFallbackServerAndTelemetryExecutableIsRun)
{
    const auto policy = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
</primary_location>
<cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>"
<cloud_subscriptions>"
    <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>"
</cloud_subscriptions>"
</AUConfig>
<server_names>
</server_names>
<Features>
    <Feature id="CORE"/>
    <Feature id="SDU"/>
</Features>
</AUConfigurations>
)sophos";

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_telemetrySchedulerStatusFilePath,
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetrySchedulerStatusFilePath, _)).WillRepeatedly(Return());
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return("machineId"));

    auto queue = std::make_shared<TaskQueue>();

    // add policy task
    // it should write config
    // wait for telemetry executable to run
    // send the shutdown task

    {
        InSequence seq;

        EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _))
            .WillOnce(Invoke(
                [](const std::string&, const std::string& content)
                {
                    auto j = nlohmann::json::parse(content);
                    EXPECT_EQ(j["server"], "t1.sophosupd.com");
                }));

        EXPECT_CALL(*m_mockProcess, setOutputLimit(_));
        EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath }));

        EXPECT_CALL(*m_mockProcess, getStatus())
            .WillOnce(InvokeWithoutArgs(
                [&queue]()
                {
                    queue->push({ SchedulerTask::TaskType::Shutdown });
                    return Common::Process::ProcessStatus::FINISHED;
                }));
        EXPECT_CALL(*m_mockProcess, exitCode()).WillOnce(Return(0));
    }

    auto pathManager = std::make_shared<Common::ApplicationConfigurationImpl::ApplicationPathManager>();
    DerivedSchedulerProcessor processor(queue, *pathManager, 0s, 0s);

    std::thread processorThread([&] {
                                    processor.run();
                                });

    queue->push({ .taskType = SchedulerTask::TaskType::Policy, .content = policy, .appId = "ALC" });
    processorThread.join();
}

// Tests for checking URL validation
TEST_F(SchedulerProcessorTests, AfterRecevingAlcPolicyWithNonUTF8TelemetryHostnameExeConfigIsNotWrittenAndTelemetryExecutableIsNotRun)
{
    const auto policy = generateAlcPolicyWithTelemetryHostname(generateNonUTF8String());

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_telemetrySchedulerStatusFilePath,
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return("machineId"));

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _)).Times(0);
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath })).Times(0);

    auto queue = std::make_shared<TaskQueue>();
    auto pathManager = std::make_shared<Common::ApplicationConfigurationImpl::ApplicationPathManager>();
    DerivedSchedulerProcessor processor(queue, *pathManager, 0s, 0s);

    std::thread processorThread([&] {
                                    processor.run();
                                });

    queue->push({ .taskType = SchedulerTask::TaskType::Policy, .content = policy, .appId = "ALC" });

    EXPECT_TRUE(waitForLog("ERROR - Failed to parse ALC policy: Error parsing xml: not well-formed (invalid token)"));
    queue->push({ SchedulerTask::TaskType::Shutdown });

    processorThread.join();
}

class SchedulerProcessorInvalidHostnameTests : public SchedulerProcessorTests, public testing::WithParamInterface<std::string>
{
    using SchedulerProcessorTests::SchedulerProcessorTests;
};

TEST_P(SchedulerProcessorInvalidHostnameTests, AfterRecevingAlcPolicyWithInvalidTelemetryHostnameExeConfigIsNotWrittenAndTelemetryExecutableIsNotRun)
{
    const auto policy = generateAlcPolicyWithTelemetryHostname(GetParam());

    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetrySchedulerStatusFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_telemetryConfigFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(
        *m_mockFileSystem,
        readFile(
            m_telemetrySchedulerStatusFilePath,
            Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(R"({"scheduled-time":1559556823})"));
    EXPECT_CALL(
        *m_mockFileSystem, readFile(m_telemetryConfigFilePath, Common::TelemetryConfigImpl::DEFAULT_MAX_JSON_SIZE))
        .WillRepeatedly(Return(m_validTelemetryConfigJson));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/machine_id.txt")).WillRepeatedly(Return("machineId"));

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_telemetryExeConfigFilePath, _)).Times(0);
    EXPECT_CALL(*m_mockProcess, exec(m_telemetryExecutableFilePath, std::vector{ m_telemetryExeConfigFilePath })).Times(0);

    auto queue = std::make_shared<TaskQueue>();
    auto pathManager = std::make_shared<Common::ApplicationConfigurationImpl::ApplicationPathManager>();
    DerivedSchedulerProcessor processor(queue, *pathManager, 0s, 0s);

    std::thread processorThread([&] {
                                    processor.run();
                                });

    queue->push({ .taskType = SchedulerTask::TaskType::Policy, .content = policy, .appId = "ALC" });

    EXPECT_TRUE(waitForLog("DEBUG - Got empty host from ALC policy"));
    queue->push({ SchedulerTask::TaskType::Shutdown });

    processorThread.join();
}

INSTANTIATE_TEST_SUITE_P(
    SchedulerProcessorInvalidHostnameTestsInstantiation,
    SchedulerProcessorInvalidHostnameTests,
    testing::Values(
        "hi@sophosupd.com",
        std::string(2000, 'a') + ".sophosupd.com",
        "sus.ophos.com",
        "sus.ophos.net",
        "sus.ophos.us"));