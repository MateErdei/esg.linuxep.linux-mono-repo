// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"

#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

// test includes
#include "tests/common/ExpectedFanotifyInitFlags.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockSysCalls.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

#include <sstream>

using namespace ::testing;
namespace fs = sophos_filesystem;

namespace
{
    constexpr unsigned int EXPECTED_FILE_EVENT_FLAGS = O_RDONLY | O_CLOEXEC | O_LARGEFILE;

    class TestFanotifyHandler : public FanotifyHandlerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            mockSysCallWrapper_ = std::make_shared<StrictMock<MockSystemCallWrapper>>();

            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            testDir_ = fs::temp_directory_path();
            testDir_ /= test_info->test_case_name();
            testDir_ /= test_info->name();
            fs::remove_all(testDir_);
            fs::create_directories(testDir_ / "var");

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, testDir_ );
            appConfig.setData("PLUGIN_INSTALL", testDir_ );

            onAccessUnhealthyFlagFile_ = Plugin::getOnAccessUnhealthyFlagPath();

            mockFileSystem_ = std::make_unique<StrictMock<MockFileSystem>>();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(testDir_);
        }

        void expectGoodHealth(int removeCalls = 2)
        {
            EXPECT_CALL(*mockFileSystem_, writeFile(onAccessUnhealthyFlagFile_, "")).Times(0);
            //removeFile gets called on destruction and successful init
            EXPECT_CALL(*mockFileSystem_, removeFile(onAccessUnhealthyFlagFile_, true)).Times(removeCalls);
            scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));
        }

        std::shared_ptr<MockSystemCallWrapper> mockSysCallWrapper_;
        std::unique_ptr<MockFileSystem> mockFileSystem_;
        fs::path testDir_;
        std::string onAccessUnhealthyFlagFile_;
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem_;
    };
}

TEST_F(TestFanotifyHandler, construction)
{
    expectGoodHealth(1);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    EXPECT_EQ(handler.getFd(), -1);
}

TEST_F(TestFanotifyHandler, testInit)
{
    int fanotifyFd = 0;
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectGoodHealth();
    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);

    std::stringstream logMsg;
    logMsg << "Fanotify successfully initialised: Fanotify FD=" << fanotifyFd;
    EXPECT_TRUE(appenderContains(logMsg.str()));
    EXPECT_FALSE(appenderContains("Unable to initialise fanotify:"));
}

TEST_F(TestFanotifyHandler, successfulInitResetsHealth)
{
    int fanotifyFd = 0;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));
}

TEST_F(TestFanotifyHandler, init_throwsErrorIfFanotifyInitFails)
{
    int fanotifyFd = -1;

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    EXPECT_CALL(*mockFileSystem_, removeFile(onAccessUnhealthyFlagFile_, true)).Times(1);
    EXPECT_CALL(*mockFileSystem_, writeFile(onAccessUnhealthyFlagFile_, "")).Times(1);
    scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    EXPECT_THROW(handler.init(), std::runtime_error);
}

TEST_F(TestFanotifyHandler, close_removesUnhealthyFlagFile)
{
    int fanotifyFd = -1;
    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    {
        InSequence seq;
        EXPECT_CALL(*mockFileSystem_, writeFile(onAccessUnhealthyFlagFile_, "")).Times(1);
        EXPECT_CALL(*mockFileSystem_, removeFile(onAccessUnhealthyFlagFile_, true)).Times(2);
    };
    scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    EXPECT_THROW(handler.init(), std::runtime_error);

    EXPECT_NO_THROW(handler.close());
}

TEST_F(TestFanotifyHandler, destructor_removesUnhealthyFlagFile)
{
    int fanotifyFd = -1;
    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    {
        InSequence seq;
        EXPECT_CALL(*mockFileSystem_, writeFile(onAccessUnhealthyFlagFile_, "")).Times(1);
        EXPECT_CALL(*mockFileSystem_, removeFile(onAccessUnhealthyFlagFile_, true)).Times(1);
    };
    scopedReplaceFileSystem_.replace(std::move(mockFileSystem_));

    {
        sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
        EXPECT_THROW(handler.init(), std::runtime_error);
    }
}

TEST_F(TestFanotifyHandler, cacheFdReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", false), 0);
}

TEST_F(TestFanotifyHandler, cacheFdWithSurviveModify)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_IGNORED_MASK | FAN_MARK_IGNORED_SURV_MODIFY,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", true), 0);
}

TEST_F(TestFanotifyHandler, errorWhenCacheFdFails)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", false), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in cacheFd: File exists for: /expected"));
}

TEST_F(TestFanotifyHandler, cacheFdWithoutInit)
{
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);
    expectGoodHealth(1);
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);

    EXPECT_EQ(handler.cacheFd(fileFd, "/expected", false), 0);
    EXPECT_TRUE(appenderContains("Skipping cacheFd for /expected as fanotify disabled"));
}

TEST_F(TestFanotifyHandler, uncacheFdReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(handler.uncacheFd(fileFd, "/expected"), 0);
}

TEST_F(TestFanotifyHandler, errorWhenUncacheFdFails)
{
    int fanotifyFd = 123;
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_IGNORED_MASK,
                                                     FAN_OPEN, fileFd, nullptr)).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(handler.uncacheFd(fileFd, "/expected"), -1);
}

TEST_F(TestFanotifyHandler, uncacheFdWithoutInit)
{
    int fileFd = 54;
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectGoodHealth(1);
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);

    EXPECT_EQ(handler.uncacheFd(fileFd, "/expected"), 0);
    EXPECT_TRUE(appenderContains("Skipping uncacheFd for /expected as fanotify disabled"));
}

TEST_F(TestFanotifyHandler, markMountReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
        SetErrnoAndReturn(0, 0));

    expectGoodHealth();
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_EQ(handler.markMount(path, true, true), 0);
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFails)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));

    expectGoodHealth();
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(handler.markMount(path, true, true), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: File exists for: /expected"));
    EXPECT_TRUE(appenderContains("[ERROR] fanotify_mark failed in markMount: File exists for: /expected"));
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFailsWithENOENT)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(ENOENT, -1));

    EXPECT_EQ(handler.markMount(path, true, true), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: No such file or directory for: /expected"));
    EXPECT_TRUE(appenderContains("[DEBUG] fanotify_mark failed in markMount: No such file or directory for: /expected"));
}

TEST_F(TestFanotifyHandler, errorWhenmarkMountFailsWithEACCES)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EACCES, -1));

    EXPECT_EQ(handler.markMount(path, true, true), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in markMount: Permission denied for: /expected"));
    EXPECT_TRUE(appenderContains("[WARN] fanotify_mark failed in markMount: Permission denied for: /expected"));
}

TEST_F(TestFanotifyHandler, markMountWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";
    expectGoodHealth(1);
    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);

    EXPECT_EQ(handler.markMount(path, true, true), 0);
    EXPECT_TRUE(appenderContains("Skipping markMount for " + path + " as fanotify disabled"));
}

TEST_F(TestFanotifyHandler, unmarkMountReturnsZeroForSuccess)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(handler.unmarkMount(path), 0);
}

TEST_F(TestFanotifyHandler, errorWhenunmarkMountFails)
{
    int fanotifyFd = 123;
    std::string path = "/expected";
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_REMOVE | FAN_MARK_MOUNT,
                                                     FAN_CLOSE_WRITE | FAN_OPEN, FAN_NOFD, path.c_str())).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(handler.unmarkMount(path), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in unmarkMount: File exists for: /expected"));
}

TEST_F(TestFanotifyHandler, unmarkMountWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "/expected";
    expectGoodHealth(1);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);

    EXPECT_EQ(handler.unmarkMount(path), 0);
    EXPECT_TRUE(appenderContains("Skipping unmarkMount for " + path + " as fanotify disabled"));
}

TEST_F(TestFanotifyHandler, clearCacheWithoutInit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    expectGoodHealth(1);

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);

    EXPECT_NO_THROW(handler.updateComplete());
}

TEST_F(TestFanotifyHandler, clearCacheSucceeds)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_FLUSH,
                                                     0, FAN_NOFD, nullptr)).WillOnce(
            SetErrnoAndReturn(0, 0));

    EXPECT_EQ(handler.clearCachedFiles(), 0);
}

TEST_F(TestFanotifyHandler, clearCacheFails)
{
    int fanotifyFd = 123;
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                                     EXPECTED_FILE_EVENT_FLAGS)).WillOnce(Return(fanotifyFd));
    expectGoodHealth();

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(mockSysCallWrapper_);
    handler.init();
    EXPECT_EQ(handler.getFd(), fanotifyFd);
    EXPECT_TRUE(appenderContains("Fanotify successfully initialised"));

    EXPECT_CALL(*mockSysCallWrapper_, fanotify_mark(fanotifyFd, FAN_MARK_FLUSH,
                                                     0, FAN_NOFD, nullptr)).WillOnce(
            SetErrnoAndReturn(EEXIST, -1));

    EXPECT_EQ(handler.clearCachedFiles(), -1);
    EXPECT_TRUE(appenderContains("fanotify_mark failed in clearCachedFiles: File exists for: fanotify fd"));;
}