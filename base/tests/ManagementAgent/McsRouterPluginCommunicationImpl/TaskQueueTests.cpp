/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "ManagementAgent/McsRouterPluginCommunicationImpl/TaskQueueProcessor.h"

#include "TempDir.h"
#include "MockPluginManager.h"

using namespace ManagementAgent::McsRouterPluginCommunicationImpl;


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

    std::unique_ptr<TaskQueueProcessor> taskQueueProcessor = std::unique_ptr<TaskQueueProcessor>(new TaskQueueProcessor(m_mockPluginManager, directoriesToWatch));

    taskQueueProcessor->start();


    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp1), tempDir.absPath(file1));
    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp2), tempDir.absPath(file2));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    taskQueueProcessor->stop();
    taskQueueProcessor->join();

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


    std::unique_ptr<TaskQueueProcessor> taskQueueProcessor = std::unique_ptr<TaskQueueProcessor>(new TaskQueueProcessor(m_mockPluginManager, directoriesToWatch));

    taskQueueProcessor->start();


    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp1), tempDir.absPath(file1));
    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp2), tempDir.absPath(file2));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    taskQueueProcessor->stop();
    taskQueueProcessor->join();

}

TEST_F(TaskQueueTests, TaskQueueProcessorCanProcessPoliciesFromMultipleDirectoriesAndWillNotThrowForUnknownFilesWithoutHyphen)
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
    std::string file2 = Common::FileSystem::fileSystem()->join(filePath2, "randomFile.txt");
    std::string fileTmp1 = Common::FileSystem::fileSystem()->join(filePath, "FileTmp1.txt");
    std::string fileTmp2 = Common::FileSystem::fileSystem()->join(filePath2, "FileTmp2.txt");

    tempDir.createFile(fileTmp1, "Hello");
    tempDir.createFile(fileTmp2, "Hello");

    MockPluginManager m_mockPluginManager;

    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "Hello")).WillOnce(Return(1));


    std::unique_ptr<TaskQueueProcessor> taskQueueProcessor = std::unique_ptr<TaskQueueProcessor>(new TaskQueueProcessor(m_mockPluginManager, directoriesToWatch));

    taskQueueProcessor->start();


    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp1), tempDir.absPath(file1));
    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp2), tempDir.absPath(file2));

    // need to give enough time for the Directory watcher to detect and process files.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    taskQueueProcessor->stop();
    taskQueueProcessor->join();

}

//TEST_F(TaskQueueTests, TaskQueueProcessorCannotProcessPoliciesFromDirectoryThatHasBeenDeletedAndWillNotThrow)
//{
//    std::vector<std::string> directoriesToWatch;
//
//    std::string filePath = "tmp/base/policy"; //"/opt/sophos-sspl/base/policy"
//    //std::string filePath2 = "tmp/base/policy2"; //"/opt/sophos-sspl/base/policy"
//    Tests::TempDir tempDir;
//
//    tempDir.makeDirs(filePath);
//    tempDir.makeDirs(filePath2);
//
//    std::string fullPath = tempDir.absPath(filePath);
//    std::string fullPath2 = tempDir.absPath(filePath2);
//
//    directoriesToWatch.push_back(fullPath);
//    directoriesToWatch.push_back(fullPath2);
//
//    std::string file1 = Common::FileSystem::fileSystem()->join(filePath, "appId1-policy.txt");
//    //std::string file2 = Common::FileSystem::fileSystem()->join(filePath2, "appId2-unknown.txt");
//    std::string fileTmp1 = Common::FileSystem::fileSystem()->join(filePath, "FileTmp1.txt");
//    //std::string fileTmp2 = Common::FileSystem::fileSystem()->join(filePath2, "FileTmp2.txt");
//
//    tempDir.createFile(fileTmp1, "Hello");
//    tempDir.createFile(fileTmp2, "Hello");
//
//    MockPluginManager m_mockPluginManager;
//
//    EXPECT_CALL(m_mockPluginManager, applyNewPolicy("appId1", "Hello")).WillOnce(Return(1));
//
//
//    std::unique_ptr<TaskQueueProcessor> taskQueueProcessor = std::unique_ptr<TaskQueueProcessor>(new TaskQueueProcessor(m_mockPluginManager, directoriesToWatch));
//
//    taskQueueProcessor->start();
//
//
//    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp1), tempDir.absPath(file1));
//    Common::FileSystem::fileSystem()->moveFile(tempDir.absPath(fileTmp2), tempDir.absPath(file2));
//
//    // need to give enough time for the Directory watcher to detect and process files.
//    std::this_thread::sleep_for(std::chrono::milliseconds(20));
//
//    taskQueueProcessor->stop();
//    taskQueueProcessor->join();
//
//}


