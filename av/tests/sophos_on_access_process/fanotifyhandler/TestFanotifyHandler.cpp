//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "datatypes/MockSysCalls.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"

#include <gtest/gtest.h>

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
            m_onAccessUnhealthyFlagFile = Plugin::getOnAccessUnhealthyFlagPath();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        bool onAccessUnhealthyFlagFileExists()
        {
            return m_fileSystem->isFile(m_onAccessUnhealthyFlagFile);
        }

        [[maybe_unused]] void createOnAccessUnhealthyFlagFile()
        {
            m_fileSystem->writeFile(m_onAccessUnhealthyFlagFile, "");
        }

        [[maybe_unused]] void removeOnAccessUnhealthyFlagFile()
        {
            m_fileSystem->removeFile(m_onAccessUnhealthyFlagFile);
        }

        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
        fs::path m_testDir;
        fs::path m_onAccessUnhealthyFlagFile;
        Common::FileSystem::IFileSystem* m_fileSystem = nullptr;
    };
}

TEST_F(TestFanotifyHandler, construction)
{
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    EXPECT_EQ(handler.getFd(), -1);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, successfulInitResetsHealth)
{
    int fanotifyFd = 0;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    createOnAccessUnhealthyFlagFile();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, init_throwsErrorIfFanotifyInitFails)
{
    int fanotifyFd = -1;

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    EXPECT_THROW(handler.init(), std::runtime_error);
    EXPECT_TRUE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, close_removesUnhealthyFlagFile)
{
    int fanotifyFd = -1;
    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    EXPECT_THROW(handler.init(), std::runtime_error);
    EXPECT_TRUE(onAccessUnhealthyFlagFileExists());

    EXPECT_NO_THROW(handler.close());
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, destructor_removesUnhealthyFlagFile)
{
    int fanotifyFd = -1;
    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
    {
        sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
        EXPECT_THROW(handler.init(), std::runtime_error);
        EXPECT_TRUE(onAccessUnhealthyFlagFileExists());
    }
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());

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

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", false), 0);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, cacheFdWithSurviveModify)
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

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_IGNORED_MASK | FAN_MARK_IGNORED_SURV_MODIFY,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", true), 0);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", false), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in cacheFd: File exists for: /expected"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, cacheFdWithoutInit)
{
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", false), 0);
    EXPECT_TRUE(appenderContains("Skipping cacheFd for /expected as fanotify disabled"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.uncacheFd(fileFd, "/expected"), 0);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.uncacheFd(fileFd, "/expected"), -1);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, uncacheFdWithoutInit)
{
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(handler.uncacheFd(fileFd, "/expected"), 0);
    EXPECT_TRUE(appenderContains("Skipping uncacheFd for /expected as fanotify disabled"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.markMount(path), 0);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFails)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(handler.markMount(path), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: File exists for: /expected"));
    EXPECT_TRUE(appenderContains("[ERROR] fanotify_mark failed in markMount: File exists for: /expected"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFailsWithENOENT)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(ENOENT, -1));

    EXPECT_EQ(handler.markMount(path), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: No such file or directory for: /expected"));
    EXPECT_TRUE(appenderContains("[DEBUG] fanotify_mark failed in markMount: No such file or directory for: /expected"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFailsWithEACCES)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS,
                                                     O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*m_mockSysCallWrapper, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EACCES, -1));

    EXPECT_EQ(handler.markMount(path), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: Permission denied for: /expected"));
    EXPECT_TRUE(appenderContains("[WARN] fanotify_mark failed in markMount: Permission denied for: /expected"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, markMountWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(handler.markMount(path), 0);
    EXPECT_TRUE(appenderContains("Skipping markMount for " + path + " as fanotify disabled"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.unmarkMount(path), 0);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.unmarkMount(path), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in unmarkMount: File exists for: /expected"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, unmarkMountWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_EQ(handler.unmarkMount(path), 0);
    EXPECT_TRUE(appenderContains("Skipping unmarkMount for " + path + " as fanotify disabled"));
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}

TEST_F(TestFanotifyHandler, clearCacheWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCallWrapper);

    EXPECT_NO_THROW(handler.updateComplete());
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.clearCachedFiles(), 0);
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
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

    EXPECT_EQ(handler.clearCachedFiles(), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in clearCachedFiles: File exists for: fanotify fd"));;
    EXPECT_FALSE(onAccessUnhealthyFlagFileExists());
}