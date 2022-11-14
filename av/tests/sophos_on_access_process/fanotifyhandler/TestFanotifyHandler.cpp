//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "datatypes/MockSysCalls.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

using namespace ::testing;
namespace fs = sophos_filesystem;

namespace
{
    class TestFanotifyHandler : public FanotifyHandlerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir / "var");

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );

            m_fileSystem = Common::FileSystem::fileSystem();
        }

        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
        fs::path m_testDir;
        Common::FileSystem::IFileSystem* m_fileSystem;
    };
}

TEST_F(TestFanotifyHandler, construction)
{
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    EXPECT_EQ(handler.getFd(), -1);
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, testInit)
{
    int fanotifyFd = 0;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);

    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));
    std::stringstream logMsg;
    logMsg << "Fanotify FD set to " << fanotifyFd;
    EXPECT_TRUE(appenderContains(logMsg.str()));
    EXPECT_FALSE(appenderContains("Unable to initialise fanotify:"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, init_throwsErrorIfFanotifyInitFails)
{
    int fanotifyFd = -1;

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    EXPECT_THROW(handler.init(), std::runtime_error);
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, cacheFdReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(0, handler.cacheFd(fileFd, "/expected"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, errorWhenCacheFdFails)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(-1, handler.cacheFd(fileFd, "/expected"));
    EXPECT_TRUE(appenderContains("fanotify_mark failed in cacheFd: File exists for: /expected"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, cacheFdWithoutInit)
{
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(0, handler.cacheFd(fileFd, "/expected"));
    EXPECT_TRUE(appenderContains("Skipping cacheFd for /expected as fanotify disabled"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, uncacheFdReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(0, handler.uncacheFd(fileFd, "/expected"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, errorWhenUncacheFdFails)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(-1, handler.uncacheFd(fileFd, "/expected"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, uncacheFdWithoutInit)
{
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(0, handler.uncacheFd(fileFd, "/expected"));
    EXPECT_TRUE(appenderContains("Skipping uncacheFd for /expected as fanotify disabled"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, markMountReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));
    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
        SetErrnoAndReturn(0, 0));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_EQ(0, handler.markMount(path));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFails)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(-1, handler.markMount(path));
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: File exists for: /expected"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, markMountWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(0, handler.markMount(path));
    EXPECT_TRUE(appenderContains("Skipping markMount for " + path + " as fanotify disabled"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, unmarkMountReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(0, handler.unmarkMount(path));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, errorWhenunmarkMountFails)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(-1, handler.unmarkMount(path));
    EXPECT_TRUE(appenderContains("fanotify_mark failed in unmarkMount: File exists for: /expected"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, unmarkMountWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(0, handler.unmarkMount(path));
    EXPECT_TRUE(appenderContains("Skipping unmarkMount for " + path + " as fanotify disabled"));
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, clearCacheWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_NO_THROW(handler.updateComplete());
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, clearCacheSucceeds)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_FLUSH,
                                                     0, FAN_NOFD, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(0, handler.clearCachedFiles());
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}

TEST_F(TestFanotifyHandler, clearCacheFails)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_FLUSH,
                                                     0, FAN_NOFD, nullptr)).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(-1, handler.clearCachedFiles());
    EXPECT_TRUE(appenderContains("fanotify_mark failed in clearCachedFiles: File exists for: fanotify fd"));;
    EXPECT_FALSE(m_fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath()));
}
