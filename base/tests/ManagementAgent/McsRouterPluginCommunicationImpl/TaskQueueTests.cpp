/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/TaskQueue/ITaskProcessor.h>
#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/TaskDirectoryListener.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>
#include <modules/ManagementAgent/McsRouterPluginCommunicationImpl/PolicyAndActionListener.h>

#include "TempDir.h"
#include "MockPluginManager.h"
#include "MockTaskQueue.h"

class TaskQueueTests : public ::testing::Test
{
public:

    void SetUp() override
    {

    }

    void TearDown() override
    {

    }



};

TEST_F(TaskQueueTests, TaskQueueProcessorCanProcessPoliciesFromMultipleDirectories)
{
    std::vector<std::string> directoriesToWatch;

    std::string filePath = "tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
    std::string filePath2 = "tmp/base/policy2"; //"/opt/sophos-sspl/base/policy"
    Tests::TempDir tempDir;

    tempDir.makeDirs(filePath);
    tempDir.makeDirs(filePath2);

    std::string fullPath = tempDir.absPath(filePath);
    std::string fullPath2 = tempDir.absPath(filePath2);

    directoriesToWatch.push_back(fullPath);
    directoriesToWatch.push_back(fullPath2);

    std::string file1 = Common::FileSystem::fileSystem()->join(filePath, "appId1-policy.txt");
    std::string file2 = Common::FileSystem::fileSystem()->join(filePath2, "appId2-policy.txt");
    std::string fileTmp1 = Common::FileSystem::fileSystem()->join(filePath, "FileTmp1.txt");
    std::string fileTmp2 = Common::FileSystem::fileSystem()->join(filePath2, "FileTmp2.txt");

    tempDir.createFile(fileTmp1, "Hello");
    tempDir.createFile(fileTmp2, "Hello");

    MockPluginManager m_mockPluginManager;

    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "Hello")).WillOnce(Return(1));
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId2", "Hello")).WillOnce(Return(1));

    std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue(
            new Common::TaskQueueImpl::TaskQueueImpl()
            );

    std::unique_ptr<Common::TaskQueue::ITaskProcessor> taskQueueProcessor(
            new Common::TaskQueueImpl::TaskProcessorImpl(taskQueue));

    taskQueueProcessor->start();

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(fullPath,taskQueue,m_mockPluginManager));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener2(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(fullPath2,taskQueue,m_mockPluginManager));

    std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> directoryWatcher(new Common::DirectoryWatcherImpl::DirectoryWatcher());

    directoryWatcher->addListener(*listener1);
    directoryWatcher->addListener(*listener2);

    directoryWatcher->startWatch();


    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp1), tempDir.absPath(file1));
    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp2), tempDir.absPath(file2));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    directoryWatcher.reset();
    taskQueueProcessor->stop();

}

TEST_F(TaskQueueTests, TaskQueueProcessorCanProcessPoliciesFromMultipleDirectoriesAndWillNotThrowForUnknownFiles)
{
    std::vector<std::string> directoriesToWatch;

    std::string filePath = "tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
    std::string filePath2 = "tmp/base/policy2"; //"/opt/sophos-sspl/base/policy"
    Tests::TempDir tempDir;

    tempDir.makeDirs(filePath);
    tempDir.makeDirs(filePath2);

    std::string fullPath = tempDir.absPath(filePath);
    std::string fullPath2 = tempDir.absPath(filePath2);

    directoriesToWatch.push_back(fullPath);
    directoriesToWatch.push_back(fullPath2);

    std::string file1 = Common::FileSystem::fileSystem()->join(filePath, "appId1-policy.txt");
    std::string file2 = Common::FileSystem::fileSystem()->join(filePath2, "appId2-unknown.txt");
    std::string fileTmp1 = Common::FileSystem::fileSystem()->join(filePath, "FileTmp1.txt");
    std::string fileTmp2 = Common::FileSystem::fileSystem()->join(filePath2, "FileTmp2.txt");

    tempDir.createFile(fileTmp1, "Hello");
    tempDir.createFile(fileTmp2, "Hello");

    MockPluginManager m_mockPluginManager;

    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "Hello")).WillOnce(Return(1));


    std::shared_ptr<Common::TaskQueue::ITaskQueue> taskQueue(
            new Common::TaskQueueImpl::TaskQueueImpl()
    );

    std::unique_ptr<Common::TaskQueue::ITaskProcessor> taskQueueProcessor(
            new Common::TaskQueueImpl::TaskProcessorImpl(taskQueue));

    taskQueueProcessor->start();

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(fullPath,taskQueue,m_mockPluginManager));

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener2(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(fullPath2,taskQueue,m_mockPluginManager));

    std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> directoryWatcher(new Common::DirectoryWatcherImpl::DirectoryWatcher());

    directoryWatcher->addListener(*listener1);
    directoryWatcher->addListener(*listener2);

    directoryWatcher->startWatch();


    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp1), tempDir.absPath(file1));
    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp2), tempDir.absPath(file2));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    directoryWatcher.reset();
    taskQueueProcessor->stop();

}

TEST(TestTaskDirectoryListener, CheckListenerThrowsAwayUnknownFiles)
{

    std::string filePath = "/tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
    std::string filename = "appId1-unknown.txt";
    std::string file1 = Common::FileSystem::fileSystem()->join(filePath, filename);

    MockPluginManager mockPluginManager;
    std::shared_ptr<MockTaskQueue> mockTaskQueue(new MockTaskQueue());

    EXPECT_CALL(*mockTaskQueue, queueTask(_)).Times(0);

    std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener>
            listener1(new ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener(filePath,mockTaskQueue,mockPluginManager));

    listener1->fileMoved(filename);
}

TEST(PolicyTask, PolicyTaskAssignsPolicyWhenRun) // NOLINT
{
    MockPluginManager m_mockPluginManager;
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("SAV", "Hello")).WillOnce(Return(1));

    auto filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, basename(_)).WillOnce(Return("SAV-11.xml"));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("Hello"));
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));


    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(m_mockPluginManager,"/tmp/policy/SAV-11.xml");
    task.run();

    Common::FileSystem::restoreFileSystem();
}

TEST(PolicyTask, PolicyTaskHandlesNameWithoutHyphen) // NOLINT
{
    MockPluginManager m_mockPluginManager;
    EXPECT_CALL(m_mockPluginManager, applyNewPolicy(_,_)).Times(0);

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyTask task(m_mockPluginManager,"/tmp/policy/PolicyTaskHandlesNameWithoutHyphen");
    task.run();
}

TEST(TestPolicyAndActionListener, Construction) // NOLINT
{
    Tests::TempDir tempDir;

    tempDir.makeDirs("policy");
    tempDir.makeDirs("action");

    MockPluginManager mockPluginManager;
    auto directoryWatcher = std::make_shared<Common::DirectoryWatcherImpl::DirectoryWatcher>();
    auto mockTaskQueue = std::make_shared<MockTaskQueue>();

    ManagementAgent::McsRouterPluginCommunicationImpl::PolicyAndActionListener foo(
            tempDir.dirPath(),
            directoryWatcher,
            mockTaskQueue,
            mockPluginManager
            );
}
