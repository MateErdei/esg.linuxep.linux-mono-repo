/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <cstring>
#include "gmock/gmock.h"
#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include "Common/DirectoryWatcher/IDirectoryWatcherException.h"
#include "Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "TestHelpers/TempDir.h"
#include "DummyDirectoryWatcherListener.h"

using namespace Common::DirectoryWatcherImpl;
using namespace ::testing;

class RealiNotifyDirectoryWatcherTests : public ::testing::Test
{
public:
    RealiNotifyDirectoryWatcherTests()
            : m_Filename1("blah123456789012345678901234567890"), m_Filename2("123456789012345")
    {
        m_TempDir1Ptr = Tests::TempDir::makeTempDir("temp1");
        m_TempDir2Ptr = Tests::TempDir::makeTempDir("temp2");
        m_Listener1Ptr = std::unique_ptr<DirectoryWatcherListener>(
                new DirectoryWatcherListener(m_TempDir1Ptr->dirPath()));
        m_Listener2Ptr = std::unique_ptr<DirectoryWatcherListener>(
                new DirectoryWatcherListener(m_TempDir2Ptr->dirPath()));
        m_WatcherPtr = std::unique_ptr<DirectoryWatcher>(new DirectoryWatcher());
        m_WatcherPtr->addListener((*m_Listener1Ptr));
        m_WatcherPtr->addListener((*m_Listener2Ptr));
        m_WatcherPtr->startWatch();
        m_TempDir1Ptr->createFile(m_Filename1, "blah1");
        m_TempDir2Ptr->createFile(m_Filename2, "blah2");
    }

    ~RealiNotifyDirectoryWatcherTests() override
    {
        m_WatcherPtr.reset();
    }

    std::unique_ptr<Tests::TempDir> m_TempDir1Ptr, m_TempDir2Ptr;
    std::unique_ptr<DirectoryWatcherListener> m_Listener1Ptr, m_Listener2Ptr;
    std::unique_ptr<DirectoryWatcher> m_WatcherPtr;
    std::string m_Filename1, m_Filename2;
    Common::FileSystem::FileSystemImpl m_FileSystem;
};


TEST_F(RealiNotifyDirectoryWatcherTests, FileCreationDoesNotTriggerEvent)
{
    usleep(1000);
    //File creation shouldn't trigger an event
    EXPECT_EQ("", m_Listener1Ptr->popFile());
    EXPECT_EQ("", m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, MoveFilesBetweenDirectoriesTriggersEvents)
{
    m_FileSystem.moveFile(m_TempDir1Ptr->absPath(m_Filename1),
                          m_TempDir2Ptr->absPath(m_Filename1));
    m_FileSystem.moveFile(m_TempDir2Ptr->absPath(m_Filename2),
                          m_TempDir1Ptr->absPath(m_Filename2));
    int retries = 0;
    while (!(m_Listener1Ptr->hasData() && m_Listener2Ptr->hasData()) && retries < 1000)
    {
        retries++;
        usleep(1000);
    }
    //File move should trigger an event
    EXPECT_EQ(m_Filename2, m_Listener1Ptr->popFile());
    EXPECT_EQ(m_Filename1, m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, DeleteListenerAndMoveFilesOnlyTriggersEventOnRemainingListener)
{
    m_WatcherPtr->removeListener((*m_Listener2Ptr));
    m_FileSystem.moveFile(m_TempDir1Ptr->absPath(m_Filename1),
                          m_TempDir2Ptr->absPath(m_Filename1));
    m_FileSystem.moveFile(m_TempDir2Ptr->absPath(m_Filename1),
                          m_TempDir1Ptr->absPath(m_Filename1));
    int retries = 0;
    while (!m_Listener1Ptr->hasData() && retries < 1000)
    {
        retries++;
        usleep(1000);
    }
    //File move should trigger an event
    EXPECT_EQ(m_Filename1, m_Listener1Ptr->popFile());
    EXPECT_EQ("", m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, DeleteFilesDoesNotTriggerEvent)
{
    remove(m_TempDir2Ptr->absPath(m_Filename1).c_str());
    remove(m_TempDir1Ptr->absPath(m_Filename2).c_str());
    usleep(1000);
    //File remove shouldn't trigger an event
    EXPECT_EQ("", m_Listener1Ptr->popFile());
    EXPECT_EQ("", m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, OverwriteExistingFileWithMoveTriggersEvent)
{
    m_FileSystem.moveFile(m_TempDir1Ptr->absPath(m_Filename1),
                          m_TempDir2Ptr->absPath(m_Filename2));
    int retries = 0;
    while (!m_Listener2Ptr->hasData() && retries < 1000)
    {
        retries++;
        usleep(1000);
    }
    //File move should trigger an event
    EXPECT_EQ(m_Filename2, m_Listener2Ptr->popFile());
    EXPECT_EQ("", m_Listener1Ptr->popFile());
}