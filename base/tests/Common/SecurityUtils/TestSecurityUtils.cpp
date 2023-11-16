// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include <climits>
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/SecurityUtils/ProcessSecurityUtils.h"
#include "Common/FileSystem/IFileSystemException.h"
#include <gtest/gtest.h>
#include "tests/Common/Helpers/TempDir.h"
#include "tests/Common/Helpers/TestMacros.h"
#include <fstream>
#include "Common/FileSystemImpl/FilePermissionsImpl.h"

using namespace Common::SecurityUtils;

class TestSecurityUtils : public ::testing::Test
{
    public:
    std::vector<std::pair<std::string,int>> m_out;

    std::string getOutput()
    {
        std::ostringstream ost;
        for (const auto& [line, level] : m_out)
        {
            ost << line << '\n';
        }
        return ost.str();
    }

};

TEST_F(TestSecurityUtils, check_user_exists)
{
    auto userNobody = getUserIdAndGroupId("lp", "lp", m_out);
    ASSERT_TRUE(userNobody) << "getUserIdAndGroupId failed: "  << getOutput();
}

TEST_F(TestSecurityUtils, TestDropPrivilegeToNobody)
{
    auto userNobody = getUserIdAndGroupId("lp", "lp", m_out);
    ASSERT_TRUE(userNobody) << "getUserIdAndGroupId failed: "  << getOutput();

    MAYSKIP;

    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(
        {
            dropPrivileges("lp", "lp", m_out);

            auto current_uid = getuid();
            auto current_gid = getgid();

            if (current_uid == userNobody->m_userid && current_gid == userNobody->m_groupid)
            {
                exit(0);
            }
            exit(2);
        },
        ::testing::ExitedWithCode(0), ".*");
}

TEST_F(TestSecurityUtils, TestDropPrivilegeLowPrivCanNotDrop)
{
    MAYSKIP;
    auto userSshd = getUserIdAndGroupId("lp", "lp", m_out);
    ASSERT_TRUE(userSshd) << "getUserIdAndGroupId failed: "  << getOutput();

    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(
        {
            dropPrivileges("lp", "lp", m_out);

            auto current_uid = getuid();
            auto current_gid = getgid();
            if (current_uid == userSshd->m_userid && current_gid == userSshd->m_groupid)
            {
                try
                {
                    dropPrivileges("lp", "lpl", m_out);
                }
                catch (FatalSecuritySetupFailureException&)
                {
                    exit(3);
                }

                exit(0);
            }
            exit(2);
        },
               ::testing::ExitedWithCode(3), ".*");
}

TEST_F(TestSecurityUtils, TestSetupJailAndGoInSetsPwdEnvVar)
{
    MAYSKIP;
    GTEST_FLAG_SET(death_test_style, "threadsafe");

    ASSERT_EXIT(
        {
            char buff[PATH_MAX] = { 0 };
            std::string oldCwd = getcwd(buff, PATH_MAX);

            setupJailAndGoIn("/tmp", m_out);

            std::string newCwd = getcwd(buff, PATH_MAX);
            std::string newPwdEnv = getenv("PWD");
            std::string newRoot = "/";
            if (oldCwd != newCwd && newPwdEnv == newCwd && newRoot == newPwdEnv)
            {
                exit(0);
            }
            exit(2);
        },
       ::testing::ExitedWithCode(0),".*");

}

TEST_F(TestSecurityUtils, TestSetupJailAndGoInNoOutsideFileAccess)
{
    MAYSKIP;
    GTEST_FLAG_SET(death_test_style, "threadsafe");

    ASSERT_EXIT(
        {
            setupJailAndGoIn("/tmp", m_out);
            // test can't see outside chroot /tmp/tempath*
            std::ifstream passwdFile("/etc/passwd");
            if (!passwdFile.is_open())
            {
                exit(0);
            }
            exit(2);
        },
        ::testing::ExitedWithCode(0),
        ".*");
}

TEST_F(TestSecurityUtils, TestSetupJailAndGoInCanCreateFilesInNewRoot)
{
    MAYSKIP;
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    Tests::TempDir tempDir("/tmp");
    std::string fileINewRoot("testNewRoot.txt");

    ASSERT_EXIT(
        {
            std::string chrootdir = tempDir.dirPath();

            setupJailAndGoIn(chrootdir, m_out);

            std::ofstream newRootTest(fileINewRoot);
            if (newRootTest.is_open())
            {
                newRootTest << "test new root\n";
                newRootTest.close();
                exit(0);
            }
            exit(2);
        },
   ::testing::ExitedWithCode(0),".*");
}


TEST_F(TestSecurityUtils, TestSetupJailAndGoInAbortsOnFailure)
{
    MAYSKIP;
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(
        {
            std::string chrootdir = ("/notpath/");
            try
            {
                setupJailAndGoIn(chrootdir, m_out);
            }
            catch (FatalSecuritySetupFailureException&)
            {
                exit(3);
            }

            exit(0);
        },
        ::testing::ExitedWithCode(3),
        ".*");
}

TEST_F(TestSecurityUtils, TestchrootAndDropPrivilegesAbortIfNotRealUser)
{
    MAYSKIP;
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(
        {
            try
            {
                chrootAndDropPrivileges("notArealUser", "notArealGrp", "/tmp", m_out);
            }
            catch (FatalSecuritySetupFailureException& fsex)
            {
                exit(3);
            }

            exit(0);
        },
        ::testing::ExitedWithCode(3),
        ".*");
}

TEST_F(TestSecurityUtils, TestChrootAndDropPrivilegesSuccessfully)
{
    MAYSKIP;

    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_EXIT(
        {
            auto userNobody = getUserIdAndGroupId("lp", "lp", m_out);
            auto nobodyUid = userNobody->m_userid;
            auto nobodyGid = userNobody->m_groupid;

            chrootAndDropPrivileges("lp", "lp", "/tmp", m_out);

            auto current_uid = getuid();
            auto current_gid = getgid();
            std::cout << "current uid: " << current_uid << "target value: " << nobodyUid << std::endl;

            // test can't see outside chroot /tmp/tempath*
            std::ifstream passwdFile("/etc/passwd");
            if (current_uid == nobodyUid && current_gid == nobodyGid && (!passwdFile.is_open()))
            {
                exit(0);
            }
            exit(2);
        },
               ::testing::ExitedWithCode(0), ".*");
}
