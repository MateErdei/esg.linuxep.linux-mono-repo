/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <modules/pluginimpl/Logger.h>

#include <modules/pluginimpl/OsqueryDataManager.h>
#include <modules/pluginimpl/QueueTask.h>
#include <Common/Helpers/TempDir.h>

using namespace Common::FileSystem;

TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}

TEST(TestLinkerWorks, WorksWithTheLogging) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");
}

TEST(TestOsqueryDataManager, rotateFileWorks)
{
    OsqueryDataManager m_DataManager;
    auto* ifileSystem = Common::FileSystem::fileSystem();
    Tests::TempDir tempdir{"/tmp"};

    std::string basefile = tempdir.dirPath()+"/file";


    ifileSystem->writeFileAtomically(basefile + ".1","blah1","/tmp");
    ifileSystem->writeFileAtomically(basefile +".2","blah2","/tmp");
    m_DataManager.rotateFiles(basefile,2);

    ASSERT_TRUE(ifileSystem->isFile(basefile + ".2"));
    ASSERT_EQ(ifileSystem->readFile(basefile + ".2"),"blah1");
    ASSERT_TRUE(ifileSystem->isFile(basefile + ".3"));
    ASSERT_EQ(ifileSystem->readFile(basefile + ".3"),"blah2");
}

TEST(TestQueueTask, popReturnsFalseWhenTImeoutReached)
{
    Plugin::QueueTask queueTask;
    Plugin::Task expectedTask;
    Plugin::Task task;
    bool return_value = queueTask.pop(task, 1);
    ASSERT_FALSE(return_value);

}

TEST(TestQueueTask, popReturnsTrueWhenTaskIsPopped)
{
    Plugin::QueueTask queueTask;
    Plugin::Task task;
    queueTask.pushStop();
    bool return_value = queueTask.pop(task, 1);
    ASSERT_TRUE(return_value);
    ASSERT_EQ(task.m_taskType,Plugin::Task::TaskType::STOP);
}

TEST(TestQueueTask, popReturnsTrueWhenTaskIsPushedDuringWait)
{
    Plugin::QueueTask queueTask;
    Plugin::Task task;
    auto pushThread = std::async(std::launch::async, [&queueTask]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queueTask.pushStop();
    });
    bool return_value = queueTask.pop(task, 1);


    ASSERT_TRUE(return_value);
    ASSERT_EQ(task.m_taskType,Plugin::Task::TaskType::STOP);
}