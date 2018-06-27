/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <sys/inotify.h>
#include <cstring>
#include "gmock/gmock.h"
#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include "Common/DirectoryWatcher/IDirectoryWatcherException.h"
#include "Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h"
#include "MockiNotifyWrapper.h"

using namespace Common::DirectoryWatcher;
using namespace ::testing;

class DirectoryWatcherListener: public Common::DirectoryWatcher::IDirectoryWatcherListener
{
public:
    explicit DirectoryWatcherListener(const std::string &path)
            : m_Path(path), m_File(""), m_Active(false), m_HasData(false)
    {}

    std::string getPath() const override
    {
        return m_Path;
    }

    void fileMoved(const std::string & filename) override
    {
        std::lock_guard<std::mutex> guard(m_FilenameMutex);
        m_File = filename;
        m_HasData = true;
    }

    void watcherActive(bool active) override
    {
        m_Active = active;
    }

    bool hasData()
    {
        return m_HasData;
    }

    std::string popFile()
    {
        std::lock_guard<std::mutex> guard(m_FilenameMutex);
        std::string data = m_File;
        m_File.clear();
        m_HasData = false;
        return data;
    }

public:
    std::string m_Path;
    std::string m_File;
    bool m_Active;
    bool m_HasData;
    std::mutex m_FilenameMutex;
};

struct MockInotifyEvent {
    int      wd;       /* Watch descriptor */
    uint32_t mask;     /* Mask describing event */
    uint32_t cookie;   /* Unique cookie associating related
                                     events (for rename(2)) */
    uint32_t len;      /* Size of name field */
    char     name[16];   /* Optional null-terminated name */
};

class DirectoryWatcherTests : public ::testing::Test
{
public:
    DirectoryWatcherTests()
    : m_Listener1("/tmp/test"), m_Listener2("/tmp/test2")
    {
        m_MockiNotifyWrapper = new StrictMock<MockiNotifyWrapper>();
        int r = pipe(m_pipe_fd);
        assert(r==0);
        EXPECT_CALL(*m_MockiNotifyWrapper, init()).WillOnce(Return(m_pipe_fd[0]));
        m_DirectoryWatcher = std::unique_ptr<DirectoryWatcher>(new DirectoryWatcher(std::unique_ptr<IiNotifyWrapper>(m_MockiNotifyWrapper)));
    }

    ~DirectoryWatcherTests() override
    {
        close(m_pipe_fd[1]);
        m_DirectoryWatcher.reset();  //Watcher must be deleted before the listeners
    }

    StrictMock<MockiNotifyWrapper>* m_MockiNotifyWrapper;
    std::unique_ptr<IDirectoryWatcher> m_DirectoryWatcher;
    DirectoryWatcherListener m_Listener1, m_Listener2;
    int m_pipe_fd[2];
    MockInotifyEvent m_MockiNotifyEvent;

};

TEST_F(DirectoryWatcherTests, failiNotifyInit) // NOLINT
{
    auto mockiNotifyWrapper = new StrictMock<MockiNotifyWrapper>();
    EXPECT_CALL(*mockiNotifyWrapper, init()).WillOnce(Return(-1));
    EXPECT_THROW(std::make_shared<DirectoryWatcher>(std::unique_ptr<IiNotifyWrapper>(mockiNotifyWrapper)), IDirectoryWatcherException);
}

TEST_F(DirectoryWatcherTests, succeediNotifyInit) // NOLINT
{
    int local_pipe[2];
    pipe(local_pipe);
    {
        auto mockiNotifyWrapper = new StrictMock<MockiNotifyWrapper>();
        EXPECT_CALL(*mockiNotifyWrapper, init()).WillOnce(Return(local_pipe[0]));
        EXPECT_NO_THROW(std::make_shared<DirectoryWatcher>(std::unique_ptr<IiNotifyWrapper>(mockiNotifyWrapper)));
    }
    close(local_pipe[1]);
}

TEST_F(DirectoryWatcherTests, failAddListenerBeforeWatch) // NOLINT
{
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, _, _)).WillOnce(Return(-1));
    EXPECT_THROW(m_DirectoryWatcher->addListener(m_Listener1), IDirectoryWatcherException);
}

TEST_F(DirectoryWatcherTests, succeedAddListenerBeforeWatch) // NOLINT
{
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, _, _)).WillOnce(Return(1));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener1));
}

TEST_F(DirectoryWatcherTests, succeedRemoveListenerBeforeWatch) // NOLINT
{
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, _, _)).WillOnce(Return(1));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener1));
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, _)).WillOnce(Return(1));
    EXPECT_NO_THROW(m_DirectoryWatcher->removeListener(m_Listener1));
}

TEST_F(DirectoryWatcherTests, failRemoveListenerBeforeWatch) // NOLINT
{
    EXPECT_THROW(m_DirectoryWatcher->removeListener(m_Listener1), IDirectoryWatcherException);
}

TEST_F(DirectoryWatcherTests, succeedRemoveListenerDuringWatch) // NOLINT
{
    m_DirectoryWatcher->startWatch();
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, _)).WillOnce(Return(1));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener1));
    EXPECT_NO_THROW(m_DirectoryWatcher->removeListener(m_Listener1));
}

TEST_F(DirectoryWatcherTests, failRemoveListenerDuringWatch) // NOLINT
{
    m_DirectoryWatcher->startWatch();
    EXPECT_THROW(m_DirectoryWatcher->removeListener(m_Listener1), IDirectoryWatcherException);
}

TEST_F(DirectoryWatcherTests, failAddListenerDuringWatch) // NOLINT
{
    m_DirectoryWatcher->startWatch();
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, _, _)).WillOnce(Return(-1));
    EXPECT_THROW(m_DirectoryWatcher->addListener(m_Listener1), IDirectoryWatcherException);
}

TEST_F(DirectoryWatcherTests, succeedAddListenerDuringWatch) // NOLINT
{
    m_DirectoryWatcher->startWatch();
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, _)).WillOnce(Return(1));  //Called by destructor
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener1));
}

TEST_F(DirectoryWatcherTests, twoListenersGetCorrectFileInfo) // NOLINT
{
    m_DirectoryWatcher->startWatch();
    std::string listener1PathString =  m_Listener1.getPath();
    std::string listener2PathString =  m_Listener2.getPath();
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, listener1PathString.c_str(), _)).WillOnce(Return(1));
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, listener2PathString.c_str(), _)).WillOnce(Return(2));
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, 1)).WillOnce(Return(1));  //Called by destructor
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, 2)).WillOnce(Return(2));  //Called by destructor
    EXPECT_CALL(*m_MockiNotifyWrapper, read(_, _, _)).WillRepeatedly(Invoke(&read));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener1));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener2));
    MockInotifyEvent inotifyEvent1 = {1, IN_MOVED_TO, 1, uint32_t(std::strlen("TestFile123.txt") + 1), "TestFile123.txt"};
    write(m_pipe_fd[1], &inotifyEvent1, sizeof(struct MockInotifyEvent));
    MockInotifyEvent inotifyEvent2 = {2, IN_MOVED_TO, 1, uint32_t(std::strlen("TestFile321.txt") + 1), "TestFile321.txt"};
    write(m_pipe_fd[1], &inotifyEvent2, sizeof(struct MockInotifyEvent));
    int retries = 0;
    while(!(m_Listener1.hasData() && m_Listener2.hasData()) && retries <1000) {
        retries ++;
        usleep(1000);
    }
    std::string file1 = m_Listener1.popFile();
    std::string file2 = m_Listener2.popFile();
    ASSERT_EQ(file1, "TestFile123.txt");
    ASSERT_EQ(file2, "TestFile321.txt");
}

TEST_F(DirectoryWatcherTests, readFailsInThread) // NOLINT
{
    m_DirectoryWatcher->startWatch();
    internal::CaptureStderr();
    std::string listener1PathString =  m_Listener1.getPath();
    std::string listener2PathString =  m_Listener2.getPath();
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, listener1PathString.c_str(), _)).WillOnce(Return(1));
    EXPECT_CALL(*m_MockiNotifyWrapper, addWatch(_, listener2PathString.c_str(), _)).WillOnce(Return(2));
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, 1)).WillOnce(Return(1));
    EXPECT_CALL(*m_MockiNotifyWrapper, removeWatch(_, 2)).WillOnce(Return(2));
    int errCode = 7;
    EXPECT_CALL(*m_MockiNotifyWrapper, read(_, _, _)).WillOnce(Invoke([&errCode](int, void*, size_t){errno = errCode; return -1;}));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener1));
    EXPECT_NO_THROW(m_DirectoryWatcher->addListener(m_Listener2));
    write(m_pipe_fd[1], "1", sizeof("1"));
    int retries = 0;
    while(m_Listener1.m_Active && m_Listener2.m_Active && retries <1000) {
        retries ++;
        usleep(1000);
    }
    std::string stdErr = internal::GetCapturedStderr();
    EXPECT_EQ(m_Listener1.m_Active, false);
    EXPECT_EQ(m_Listener2.m_Active, false);
    std::stringstream errStream;
    errStream << "iNotify read failed with error " << errCode << ": Stopping DirectoryWatcher" << std::endl;
    EXPECT_EQ(stdErr, errStream.str());
}
