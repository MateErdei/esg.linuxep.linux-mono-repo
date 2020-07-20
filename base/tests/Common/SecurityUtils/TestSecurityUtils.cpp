/******************************************************************************************************
Copyright 2020, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#include <climits>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <gtest/gtest.h>
#include "../Helpers/TempDir.h"
#include <fstream>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>

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

//
//TEST(TestSecurityUtils, TestDropPrivilegeToNobody) // NOLINT
//{
//    MAYSKIP;
//
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT({
//                    auto userNobody = getUserIdAndGroupId("nobody", "nobody");
//                    dropPrivileges("nobody", "nobody");
//
//                    auto current_uid = getuid();
//                    auto current_gid = getgid();
//                    std::cout << "actual uid: " << current_uid << "target value: " << userNobody->m_userid << std::endl;
//
//                    if (current_uid == userNobody->m_userid && current_gid == userNobody->m_groupid)
//                    {
//                        exit(0);
//                    }
//                    exit(2);
//                },
//                ::testing::ExitedWithCode(0), ".*");
//}
//
//TEST(TestSecurityUtils, TestDropPrivilegeLowPrivCanNotDrop) // NOLINT
//{
//    MAYSKIP;
//
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT({
//                    auto userSshd = getUserIdAndGroupId("sshd", "sshd");
//                    dropPrivileges("sshd", "sshd");
//
//                    auto current_uid = getuid();
//                    auto current_gid = getgid();
//                    if (current_uid == userSshd->m_userid && current_gid == userSshd->m_groupid)
//                    {
//                        dropPrivileges("nobody", "nobody");
//                        exit(0);
//                    }
//                    exit(2);
//                },
//                ::testing::ExitedWithCode(1), ".*");
//}
//
//TEST(TestSecurityUtils, TestSetupJailAndGoInSetsPwdEnvVar) // NOLINT
//{
//    MAYSKIP;
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//
//    ASSERT_EXIT({
//                    char buff[PATH_MAX] = {0};
//                    std::string oldCwd = getcwd(buff, PATH_MAX);
//
//                    std::cout << "old working dir: " << oldCwd << std::endl;
//                    setupJailAndGoIn("/tmp");
//
//
//                    std::string newCwd = getcwd(buff, PATH_MAX);
//                    std::string newPwdEnv = getenv("PWD");
//                    std::string newRoot = "/";
//                    std::cout << "new working dir: " << newCwd << std::endl;
//                    if (oldCwd != newCwd && newPwdEnv == newCwd && newRoot == newPwdEnv)
//                    {
//                        exit(0);
//                    }
//                    exit(2);
//                },
//        ::testing::ExitedWithCode(0),".*");
//
//}
//
//TEST(TestSecurityUtils, TestSetupJailAndGoInNoOutsideFileAccess) // NOLINT
//{
//    MAYSKIP;
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//
//    ASSERT_EXIT({
//        setupJailAndGoIn("/tmp");
//        //test can't see outside chroot /tmp/tempath*
//        std::ifstream passwdFile ("/etc/passwd");
//        if (!passwdFile.is_open())
//        {
//            exit(0);
//        }
//        exit(2);  },
//                ::testing::ExitedWithCode(0),".*");
//}
//
//TEST(TestSecurityUtils, TestSetupJailAndGoInCanCreateFilesInNewRoot) // NOLINT
//{
//    MAYSKIP;
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    Tests::TempDir tempDir("/tmp");
//    std::string fileINewRoot("testNewRoot.txt");
//
//    ASSERT_EXIT({
//        std::string chrootdir = tempDir.dirPath();
//
//        setupJailAndGoIn(chrootdir);
//
//        std::ofstream newRootTest(fileINewRoot);
//        if (newRootTest.is_open())
//        {
//            newRootTest << "test new root\n";
//            newRootTest.close();
//            exit(0);
//        }
//        exit(2);  },
//    ::testing::ExitedWithCode(0),".*");
//}
//
//
//TEST(TestSecurityUtils, TestSetupJailAndGoInAbortsOnFailure) // NOLINT
//{
//    MAYSKIP;
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT({
//        std::string chrootdir = ("/notpath/");
//        setupJailAndGoIn(chrootdir);
//        exit(0);
//        },
//                ::testing::ExitedWithCode(EXIT_FAILURE),".*");
//}
//
//TEST(TestSecurityUtils, TestchrootAndDropPrivilegesAbortIfNotRealUser) // NOLINT
//{
//    MAYSKIP;
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT({
//                    chrootAndDropPrivileges("notArealUser", "notArealGrp", "/tmp");
//                    exit(0);
//                },
//                ::testing::ExitedWithCode(EXIT_FAILURE), ".*");
//}
//
//TEST(TestSecurityUtils, TestChrootAndDropPrivilegesSuccessfully) // NOLINT
//{
//    MAYSKIP;
//
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT({
//                    auto userNobody = getUserIdAndGroupId("nobody", "nobody");
//                    auto nobodyUid = userNobody->m_userid;
//                    auto nobodyGid = userNobody->m_groupid;
//
//                    chrootAndDropPrivileges("nobody", "nobody", "/tmp");
//
//                    auto current_uid = getuid();
//                    auto current_gid = getgid();
//                    std::cout << "current uid: " << current_uid << "target value: " << nobodyUid << std::endl;
//
//                    //test can't see outside chroot /tmp/tempath*
//                    std::ifstream passwdFile("/etc/passwd");
//                    if (current_uid == nobodyUid && current_gid == nobodyGid && (!passwdFile.is_open())) {
//                        exit(0);
//                    }
//                    exit(2);
//                },
//                ::testing::ExitedWithCode(0), ".*");
//}


class TestSecurityUtilsBindMount : public ::testing::Test
{

public:
    TestSecurityUtilsBindMount() : m_rootPath("/tmp"), m_tempDir{m_rootPath, "TestSecurityUtils"}
    {
        testing::FLAGS_gtest_death_test_style = "threadsafe";
    }

    std::string m_rootPath;
    Tests::TempDir m_tempDir;
    std::string m_sourceDir;
    std::string m_targetDir;


    void setupAfterSkipIfNotRoot()
    {
        std::cout << "called setupAfterSkipIfNotRoot" << std::endl;

        m_tempDir.makeTempDir();
        auto fperms = Common::FileSystem::FilePermissionsImpl();
        fperms.chmod(m_tempDir.dirPath(), 0777);

        m_tempDir.makeDirs(std::vector<std::string>{
                "sourceDir", "targetDir", "sourceFile"
        });


        std::string sophosInstall = m_tempDir.dirPath();
        m_sourceDir = m_tempDir.absPath("sourceDir");
        m_targetDir = m_tempDir.absPath("targetDir");
    }
};

TEST_F(TestSecurityUtilsBindMount, TestBindMountReadOnlyMoutDir) // NOLINT
{
    MAYSKIP;

    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "test");
    bindMountReadOnly(m_sourceDir, m_targetDir);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));

    unMount(m_targetDir);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(Common::FileSystem::join(m_targetDir, GL_NOTMOUNTED_MARKER)));
}

TEST_F(TestSecurityUtilsBindMount, TestBindMountReadOnlyMoutFile) // NOLINT
{
    MAYSKIP;
    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "source");

    //Target will be marked with content SPL.GL_NOTMOUNTED_MARKER
    m_tempDir.createFile(testFileTarget, "");
    bindMountReadOnly(testFileSrc, testFileTarget);
    auto afterBindMount = Common::FileSystem::fileSystem()->readFile(testFileTarget);
    ASSERT_STREQ(afterBindMount.c_str(), "source");

    unMount(testFileTarget);
    auto afterUnmount = Common::FileSystem::fileSystem()->readFile(testFileTarget);
    ASSERT_STREQ(afterUnmount.c_str(), GL_NOTMOUNTED_MARKER);
}

TEST_F(TestSecurityUtilsBindMount, WillNotMountIfAlreadyMounted) // NOLINT
{
    MAYSKIP;

    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "test");
    bindMountReadOnly(m_sourceDir, m_targetDir);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    //This will not run mount otherwise we would require to unmount twice
    bindMountReadOnly(m_sourceDir, m_targetDir);

    //check only a single unmount if required
    unMount(m_targetDir);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
}

TEST_F(TestSecurityUtilsBindMount, WillUsePreviousilyCreatedMountTargets) // NOLINT
{
    MAYSKIP;

    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "test");

    bindMountReadOnly(m_sourceDir, m_targetDir);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));

    //check only a single unmount if required
    unMount(m_targetDir);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(Common::FileSystem::join(m_targetDir, GL_NOTMOUNTED_MARKER)));

    //Will mount over previously created file
    bindMountReadOnly(m_sourceDir, m_targetDir);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));

    unMount(m_targetDir);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
}