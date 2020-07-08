/******************************************************************************************************
Copyright 2020, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#include <climits>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <gtest/gtest.h>
#include <include/gmock/gmock-matchers.h>
#include "../Helpers/TempDir.h"
#include <fstream>
#include <sys/stat.h>

using namespace Common::SecurityUtils;

bool skipTest()
{
#ifndef NDEBUG
    std::cout << "not compatible with coverage ... skip" << std::endl;
    return true;
#else
    if (getuid() != 0)
    {
        std::cout << "requires root ... skip" << std::endl;
        return true;
    }
    return false;
#endif

}

#define MAYSKIP if(skipTest()) return

TEST(TestSecurityUtils, TestDropPrivilegeToNobody) // NOLINT
{
    MAYSKIP;

    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT({
                    auto userNobody = getUserIdAndGroupId("nobody", "nobody");
                    dropPrivileges("nobody", "nobody");

                    auto current_uid = getuid();
                    auto current_gid = getgid();
                    std::cout << "actual uid: " << current_uid << "target value: " << userNobody->m_userid << std::endl;

                    if (current_uid == userNobody->m_userid && current_gid == userNobody->m_groupid) {
                        exit(0);
                    }
                    exit(2);
                },
                ::testing::ExitedWithCode(0), ".*");
}


TEST(TestSecurityUtils, TestSetupJailAndGoInSetsPwdEnvVar) // NOLINT
{
    MAYSKIP;
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    ASSERT_EXIT({
        char buff[PATH_MAX]={0};
        std::string oldCwd = getcwd(buff, PATH_MAX);

        std::cout << "old working dir: " << oldCwd << std::endl;
        setupJailAndGoIn("/tmp");


        std::string newCwd = getcwd(buff, PATH_MAX);
        std::string newPwdEnv = getenv("PWD");
        std::string newRoot = "/";
        std::cout << "new working dir: " << newCwd << std::endl;
        if ( oldCwd != newCwd && newPwdEnv==newCwd && newRoot == newPwdEnv)
        {
            exit(0);
        }
        exit(2);  },
        ::testing::ExitedWithCode(0),".*");

}

TEST(TestSecurityUtils, TestSetupJailAndGoInNoOutsideFileAccess) // NOLINT
{
    MAYSKIP;
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    ASSERT_EXIT({
        setupJailAndGoIn("/tmp");
        //test can't see outside chroot /tmp/tempath*
        std::ifstream passwdFile ("/etc/passwd");
        if (!passwdFile.is_open())
        {
            exit(0);
        }
        exit(2);  },
                ::testing::ExitedWithCode(0),".*");
}

TEST(TestSecurityUtils, TestSetupJailAndGoInCanCreateFilesInNewRoot) // NOLINT
{
    MAYSKIP;
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    Tests::TempDir tempDir("/tmp");
    std::string fileINewRoot("testNewRoot.txt");

    ASSERT_EXIT({
        std::string chrootdir = tempDir.dirPath();

        setupJailAndGoIn(chrootdir);

        std::ofstream newRootTest(fileINewRoot);
        if (newRootTest.is_open())
        {
            newRootTest << "test new root\n";
            newRootTest.close();
            exit(0);
        }
        exit(2);  },
    ::testing::ExitedWithCode(0),".*");
}


TEST(TestSecurityUtils, TestSetupJailAndGoInAbortsOnFailure) // NOLINT
{
    MAYSKIP;
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT({
        std::string chrootdir = ("/notpath/");
        setupJailAndGoIn(chrootdir);
        exit(0);
        },
                ::testing::ExitedWithCode(EXIT_FAILURE),".*");
}

TEST(TestSecurityUtils, TestchrootAndDropPrivilegesAbortIfNotRealUser) // NOLINT
{
    MAYSKIP;
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT({
                    chrootAndDropPrivileges("notArealUser", "notArealGrp", "/tmp");
                    exit(0);
                },
                ::testing::ExitedWithCode(EXIT_FAILURE), ".*");
}

TEST(TestSecurityUtils, TestChrootAndDropPrivilegesSuccessfully) // NOLINT
{
    MAYSKIP;

    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT({
                    auto userNobody = getUserIdAndGroupId("nobody", "nobody");
                    auto nobodyUid = userNobody->m_userid;
                    auto nobodyGid = userNobody->m_groupid;

                    chrootAndDropPrivileges("nobody", "nobody", "/tmp");

                    auto current_uid = getuid();
                    auto current_gid = getgid();
                    std::cout << "current uid: " << current_uid << "target value: " << nobodyUid << std::endl;

                    //test can't see outside chroot /tmp/tempath*
                    std::ifstream passwdFile("/etc/passwd");
                    if (current_uid == nobodyUid && current_gid == nobodyGid && (!passwdFile.is_open())) {
                        exit(0);
                    }
                    exit(2);
                },
                ::testing::ExitedWithCode(0), ".*");

}