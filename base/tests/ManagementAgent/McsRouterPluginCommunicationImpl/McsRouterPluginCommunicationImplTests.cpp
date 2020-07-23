/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginManager.h"

#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/TaskQueue/ITaskProcessor.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

#include <future>

class McsRouterPluginCommunicationImplTests : public ::testing::Test
{
public:
    McsRouterPluginCommunicationImplTests() = default;

    void SetUp() override
    {
        m_policyFilePath = "tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
        m_actionFilePath = "tmp/base/action"; //"/opt/sophos-sspl/base/action"

        m_tempDir = Tests::TempDir::makeTempDir();

        m_tempDir->makeDirs(m_policyFilePath);
        m_tempDir->makeDirs(m_actionFilePath);

        m_policyFullPath = m_tempDir->absPath(m_policyFilePath);
        m_actionFullPath = m_tempDir->absPath(m_actionFilePath);

        m_directoriesToWatch.push_back(m_policyFullPath);
        m_directoriesToWatch.push_back(m_actionFullPath);

        m_taskQueue = std::make_shared<Common::TaskQueueImpl::TaskQueueImpl>();

        m_taskQueueProcessor = std::unique_ptr<Common::TaskQueue::ITaskProcessor>(
            new Common::TaskQueueImpl::TaskProcessorImpl(m_taskQueue));

        m_taskQueueProcessor->start();
    }

    void TearDown() override
    {
        m_taskQueueProcessor->stop();
        m_tempDir.reset(nullptr);
        m_taskQueueProcessor.reset(nullptr);
        m_taskQueue.reset();
    }

    std::vector<std::string> m_directoriesToWatch;

    std::string m_policyFilePath;
    std::string m_actionFilePath;
    std::unique_ptr<Tests::TempDir> m_tempDir;

    std::string m_policyFullPath;
    std::string m_actionFullPath;

    StrictMock<MockPluginManager> m_mockPluginManager;

    std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQueue;
    std::unique_ptr<Common::TaskQueue::ITaskProcessor> m_taskQueueProcessor;

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(McsRouterPluginCommunicationImplTests, TaskQueueProcessorCanProcessFilesFromMultipleDirectories) // NOLINT
{
    using namespace ::testing;
    std::string policyFile1 = Common::FileSystem::join(m_policyFilePath, "appId1-1_policy.xml");
    std::string policyFileTmp1 = Common::FileSystem::join(m_policyFilePath, "policyFileTmp1.xml");

    std::string actionFile1 = Common::FileSystem::join(m_actionFilePath, "appId1_action_.xml");
    std::string actionFileTmp1 = Common::FileSystem::join(m_actionFilePath, "actionFileTmp1.xml");

    m_tempDir->createFile(policyFileTmp1, "Hello");
    m_tempDir->createFile(actionFileTmp1, "Hello");

    MockedApplicationPathManager* mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
    Common::ApplicationConfiguration::replaceApplicationPathManager(
        std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));

    EXPECT_CALL(*mockApplicationPathManager, getMcsPolicyFilePath()).WillOnce(Return(m_tempDir->absPath(m_policyFilePath)));
    EXPECT_CALL(*mockApplicationPathManager, getMcsActionFilePath()).WillOnce(Return(m_tempDir->absPath(actionFile1)));

    Tests::TestExecutionSynchronizer sync(2);
    auto notifySync= [&sync](std::string, std::string, std::string=""){sync.notify(); return 1;};
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "appId1-1_policy.xml")).WillOnce(Invoke(notifySync));
    EXPECT_CALL(m_mockPluginManager, queueAction("appId1",  "appId1_action_.xml", "")).WillOnce(Invoke(notifySync));


    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> listener1(
        new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(
            m_policyFullPath, m_taskQueue, m_mockPluginManager));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> listener2(
        new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(
            m_actionFullPath, m_taskQueue, m_mockPluginManager));

    auto directoryWatcher = Common::DirectoryWatcher::createDirectoryWatcher();

    directoryWatcher->addListener(*listener1);
    directoryWatcher->addListener(*listener2);

    directoryWatcher->startWatch();

    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(policyFileTmp1),
        m_tempDir->absPath(policyFile1)));                      // NOLINT
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(actionFileTmp1),
        m_tempDir->absPath(actionFile1))); // NOLINT


    // wait up to one second for the sync to be executed via the notifySync
    sync.waitfor(1000);

    directoryWatcher.reset();
}

TEST_F( // NOLINT
    McsRouterPluginCommunicationImplTests,
    TaskQueueProcessorCanProcessMultipleFilesFromMultipleDirectoriesAndWillNotThrowForUnknownFiles)
{
    std::string policyFileName1("appId1_policy.xml");
    std::string policyFileName2("appId2-3_policy.xml");
    std::string policyFileName3("appId3_unknown.xml");

    std::string actionFileName1("appId1_action_.xml");
    std::string actionFileName2( "appId2_action_.xml");
    std::string actionFileName3("appId3-unknown.xml");


    std::string policyFile1 = Common::FileSystem::join(m_policyFilePath, policyFileName1);
    std::string policyFile2 = Common::FileSystem::join(m_policyFilePath, policyFileName2);
    std::string policyFile3 = Common::FileSystem::join(m_policyFilePath, policyFileName3);

    std::string actionFile1 = Common::FileSystem::join(m_actionFilePath, actionFileName1);
    std::string actionFile2 = Common::FileSystem::join(m_actionFilePath, actionFileName2);
    std::string actionFile3 = Common::FileSystem::join(m_actionFilePath, actionFileName3);

    std::string policyFileTmp1 = Common::FileSystem::join(m_policyFilePath, "policyFileTmp1.txt");
    std::string policyFileTmp2 = Common::FileSystem::join(m_policyFilePath, "policyFileTmp2.txt");
    std::string policyFileTmp3 = Common::FileSystem::join(m_policyFilePath, "policyFileTmp3.txt");

    std::string actionFileTmp1 = Common::FileSystem::join(m_actionFilePath, "actionFileTmp1.txt");
    std::string actionFileTmp2 = Common::FileSystem::join(m_actionFilePath, "actionFileTmp2.txt");
    std::string actionFileTmp3 = Common::FileSystem::join(m_actionFilePath, "actionFileTmp3.txt");

    m_tempDir->createFile(policyFileTmp1, "Hello");
    m_tempDir->createFile(policyFileTmp2, "Hello");
    m_tempDir->createFile(policyFileTmp3, "Hello");

    m_tempDir->createFile(actionFileTmp1, "Hello");
    m_tempDir->createFile(actionFileTmp2, "Hello");
    m_tempDir->createFile(actionFileTmp3, "Hello");

    MockedApplicationPathManager* mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
    Common::ApplicationConfiguration::replaceApplicationPathManager(
        std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));

    EXPECT_CALL(*mockApplicationPathManager, getMcsPolicyFilePath()).WillRepeatedly(Return(m_tempDir->absPath(m_policyFilePath)));
    EXPECT_CALL(*mockApplicationPathManager, getMcsActionFilePath()).WillRepeatedly(Return(m_tempDir->absPath(m_actionFilePath)));

    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", policyFileName1)).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId2", policyFileName2)).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId3", policyFileName3)).Times(0);

    EXPECT_CALL(m_mockPluginManager, queueAction("appId1", actionFileName1, "")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, queueAction("appId2",  actionFileName2, "")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, queueAction("appId3",  actionFileName3, "")).Times(0);

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> listener1(
        new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(
            m_policyFullPath, m_taskQueue, m_mockPluginManager));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> listener2(
        new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(
            m_actionFullPath, m_taskQueue, m_mockPluginManager));

    auto directoryWatcher = Common::DirectoryWatcher::createDirectoryWatcher();

    directoryWatcher->addListener(*listener1);
    directoryWatcher->addListener(*listener2);

    directoryWatcher->startWatch();

    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(policyFileTmp1),
        m_tempDir->absPath(policyFile1)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(policyFileTmp2),
        m_tempDir->absPath(policyFile2)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(policyFileTmp3),
        m_tempDir->absPath(policyFile3)));

    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(actionFileTmp1),
        m_tempDir->absPath(actionFile1)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(actionFileTmp2),
        m_tempDir->absPath(actionFile2)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile( // NOLINT
        m_tempDir->absPath(actionFileTmp3),
        m_tempDir->absPath(actionFile3)));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    directoryWatcher.reset();
}