/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/TaskQueue/ITaskProcessor.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>

#include "TempDir.h"
#include "MockPluginManager.h"

class McsRouterPluginCommunicationImplTests : public ::testing::Test
{
public:

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
};

TEST_F(McsRouterPluginCommunicationImplTests, TaskQueueProcessorCanProcessFilesFromMultipleDirectories)
{
    std::string policyFile1 = Common::FileSystem::join(m_policyFilePath, "appId1-policy.txt");
    std::string policyFileTmp1 = Common::FileSystem::join(m_policyFilePath, "policyFileTmp1.txt");

    std::string actionFile1 = Common::FileSystem::join(m_actionFilePath, "appId1_action_.txt");
    std::string actionFileTmp1 = Common::FileSystem::join(m_actionFilePath, "actionFileTmp1.txt");

    m_tempDir->createFile(policyFileTmp1, "Hello");
    m_tempDir->createFile(actionFileTmp1, "Hello");

    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "Hello")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, queueAction("appId1", "Hello")).WillOnce(Return(1));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(m_policyFullPath,m_taskQueue,m_mockPluginManager));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener2(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(m_actionFullPath,m_taskQueue,m_mockPluginManager));

    std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> directoryWatcher(new Common::DirectoryWatcherImpl::DirectoryWatcher());

    directoryWatcher->addListener(*listener1);
    directoryWatcher->addListener(*listener2);

    directoryWatcher->startWatch();

    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(policyFileTmp1), m_tempDir->absPath(policyFile1)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(actionFileTmp1), m_tempDir->absPath(actionFile1)));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    directoryWatcher.reset();

}

TEST_F(McsRouterPluginCommunicationImplTests, TaskQueueProcessorCanProcessMultipleFilesFromMultipleDirectoriesAndWillNotThrowForUnknownFiles)
{
    std::string policyFile1 = Common::FileSystem::join(m_policyFilePath, "appId1-policy.txt");
    std::string policyFile2 = Common::FileSystem::join(m_policyFilePath, "appId2-policy.txt");
    std::string policyFile3 = Common::FileSystem::join(m_policyFilePath, "appId3-unknown.txt");

    std::string actionFile1 = Common::FileSystem::join(m_actionFilePath, "appId1_action_.txt");
    std::string actionFile2 = Common::FileSystem::join(m_actionFilePath, "appId2_action_.txt");
    std::string actionFile3 = Common::FileSystem::join(m_actionFilePath, "appId3-unknown.txt");


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

    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "Hello")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId2", "Hello")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId3", "Hello")).Times(0);

    EXPECT_CALL(m_mockPluginManager, queueAction("appId1", "Hello")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, queueAction("appId2", "Hello")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, queueAction("appId3", "Hello")).Times(0);

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(m_policyFullPath,m_taskQueue,m_mockPluginManager));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener2(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(m_actionFullPath,m_taskQueue,m_mockPluginManager));

    std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> directoryWatcher(new Common::DirectoryWatcherImpl::DirectoryWatcher());

    directoryWatcher->addListener(*listener1);
    directoryWatcher->addListener(*listener2);

    directoryWatcher->startWatch();

    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(policyFileTmp1), m_tempDir->absPath(policyFile1)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(policyFileTmp2), m_tempDir->absPath(policyFile2)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(policyFileTmp3), m_tempDir->absPath(policyFile3)));

    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(actionFileTmp1), m_tempDir->absPath(actionFile1)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(actionFileTmp2), m_tempDir->absPath(actionFile2)));
    EXPECT_NO_THROW(Common::FileSystem::fileSystem()->moveFile(m_tempDir->absPath(actionFileTmp3), m_tempDir->absPath(actionFile3)));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    directoryWatcher.reset();

}