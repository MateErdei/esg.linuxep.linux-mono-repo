/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DummyDirectoryWatcherListener.h"

#include <Common/DirectoryWatcher/IDirectoryWatcher.h>
#include <Common/DirectoryWatcher/IDirectoryWatcherException.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <gmock/gmock.h>
#include <tests/Common/Helpers/TempDir.h>

#include <cstring>

using namespace Common::DirectoryWatcherImpl;
using namespace ::testing;

class RealiNotifyDirectoryWatcherTests : public ::testing::Test
{
public:
    RealiNotifyDirectoryWatcherTests() :
        m_Filename1("blah123456789012345678901234567890"),
        m_Filename2("123456789012345")
    {
        m_TempDir1Ptr = Tests::TempDir::makeTempDir("temp1");
        m_TempDir2Ptr = Tests::TempDir::makeTempDir("temp2");
        m_Listener1Ptr =
            std::unique_ptr<DirectoryWatcherListener>(new DirectoryWatcherListener(m_TempDir1Ptr->dirPath()));
        m_Listener2Ptr =
            std::unique_ptr<DirectoryWatcherListener>(new DirectoryWatcherListener(m_TempDir2Ptr->dirPath()));
        m_WatcherPtr = std::unique_ptr<DirectoryWatcher>(new DirectoryWatcher());
        m_WatcherPtr->addListener((*m_Listener1Ptr));
        m_WatcherPtr->addListener((*m_Listener2Ptr));
        m_WatcherPtr->startWatch();
        m_TempDir1Ptr->createFile(m_Filename1, "blah1");
        m_TempDir2Ptr->createFile(m_Filename2, "blah2");
    }

    ~RealiNotifyDirectoryWatcherTests() override { m_WatcherPtr.reset(); }

    std::unique_ptr<Tests::TempDir> m_TempDir1Ptr, m_TempDir2Ptr;
    std::unique_ptr<DirectoryWatcherListener> m_Listener1Ptr, m_Listener2Ptr;
    std::unique_ptr<DirectoryWatcher> m_WatcherPtr;
    std::string m_Filename1, m_Filename2;
    Common::FileSystem::FileSystemImpl m_FileSystem;
};

namespace
{
    void removeFile(const std::string& path)
    {
        int ret = ::remove(path.c_str());
        if (ret == -1 && errno == ENOENT)
        {
            return;
        }
        ASSERT_EQ(ret, 0);
    }
} // namespace

TEST( // NOLINT
    DirectoryWatcherTest,
    LimitationOfDirectoryWatcher_IfADirectoryIsDeletedAndRecreatedDirectoryWatcherWillNotRealise)
{
    auto tempDir = Tests::TempDir::makeTempDir("temp1");
    tempDir->makeDirs("innerdir");
    auto listener1Ptr =
        std::unique_ptr<DirectoryWatcherListener>(new DirectoryWatcherListener(tempDir->absPath("innerdir")));
    auto watcherPtr = std::unique_ptr<DirectoryWatcher>(new DirectoryWatcher());
    watcherPtr->addListener((*listener1Ptr));
    watcherPtr->startWatch();
    std::string relPath = "innerdir/path1.log";
    tempDir->createFileAtomically(relPath, "any content");
    int retries = 0;
    while (!(listener1Ptr->hasData()) && retries < 1000)
    {
        retries++;
        usleep(1000);
    }

    // File creation shouldn't trigger an event
    EXPECT_EQ(listener1Ptr->popFile(), "path1.log");

    // delete the directory
    auto fsystem = Common::FileSystem::fileSystem();
    fsystem->removeFile(tempDir->absPath(relPath));
    std::string dirPath = tempDir->absPath("innerdir");
    ASSERT_EQ(::rmdir(dirPath.c_str()), 0);

    tempDir->makeDirs("innerdir");
    tempDir->createFileAtomically(relPath, "any content");
    retries = 0;
    while (!(listener1Ptr->hasData()) && retries < 100)
    {
        retries++;
        usleep(1000);
    }
    EXPECT_EQ(retries, 100);
    EXPECT_TRUE(listener1Ptr->m_Active);
}

TEST_F(RealiNotifyDirectoryWatcherTests, FileCreationDoesNotTriggerEvent) // NOLINT
{
    usleep(1000);
    // File creation shouldn't trigger an event
    EXPECT_EQ("", m_Listener1Ptr->popFile());
    EXPECT_EQ("", m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, MoveFilesBetweenDirectoriesTriggersEvents) // NOLINT
{
    m_FileSystem.moveFile(m_TempDir1Ptr->absPath(m_Filename1), m_TempDir2Ptr->absPath(m_Filename1));
    m_FileSystem.moveFile(m_TempDir2Ptr->absPath(m_Filename2), m_TempDir1Ptr->absPath(m_Filename2));
    int retries = 0;
    while (!(m_Listener1Ptr->hasData() && m_Listener2Ptr->hasData()) && retries < 1000)
    {
        retries++;
        usleep(1000);
    }
    // File move should trigger an event
    EXPECT_EQ(m_Filename2, m_Listener1Ptr->popFile());
    EXPECT_EQ(m_Filename1, m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, DeleteListenerAndMoveFilesOnlyTriggersEventOnRemainingListener) // NOLINT
{
    m_WatcherPtr->removeListener((*m_Listener2Ptr));
    m_FileSystem.moveFile(m_TempDir1Ptr->absPath(m_Filename1), m_TempDir2Ptr->absPath(m_Filename1));
    m_FileSystem.moveFile(m_TempDir2Ptr->absPath(m_Filename1), m_TempDir1Ptr->absPath(m_Filename1));
    int retries = 0;
    while (!m_Listener1Ptr->hasData() && retries < 1000)
    {
        retries++;
        usleep(1000);
    }
    // File move should trigger an event
    EXPECT_EQ(m_Filename1, m_Listener1Ptr->popFile());
    EXPECT_EQ("", m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, DeleteFilesDoesNotTriggerEvent) // NOLINT
{
    removeFile(m_TempDir2Ptr->absPath(m_Filename1));
    removeFile(m_TempDir1Ptr->absPath(m_Filename2));
    usleep(1000);
    // File remove shouldn't trigger an event
    EXPECT_EQ("", m_Listener1Ptr->popFile());
    EXPECT_EQ("", m_Listener2Ptr->popFile());
}

TEST_F(RealiNotifyDirectoryWatcherTests, OverwriteExistingFileWithMoveTriggersEvent) // NOLINT
{
    m_FileSystem.moveFile(m_TempDir1Ptr->absPath(m_Filename1), m_TempDir2Ptr->absPath(m_Filename2));
    int retries = 0;
    while (!m_Listener2Ptr->hasData() && retries < 1000)
    {
        retries++;
        usleep(1000);
    }
    // File move should trigger an event
    EXPECT_EQ(m_Filename2, m_Listener2Ptr->popFile());
    EXPECT_EQ("", m_Listener1Ptr->popFile());
}