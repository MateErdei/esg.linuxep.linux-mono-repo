/******************************************************************************************************
Copyright 2020, Sophos Limited.  All rights reserved.
******************************************************************************************************/
#include <climits>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestMacros.h>
#include <fstream>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>

using namespace Common::SecurityUtils;

class TestSecurityUtils : public ::testing::Test
{
    public:
    std::vector<std::pair<std::string,int>> m_out;
};

TEST_F(TestSecurityUtils, TestDropPrivilegeToNobody) // NOLINT
{
   MAYSKIP;

   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   ASSERT_EXIT({
                   auto userNobody = getUserIdAndGroupId("lp", "lp", m_out);
                   dropPrivileges("lp", "lp", m_out);

                   auto current_uid = getuid();
                   auto current_gid = getgid();

                   if (current_uid == userNobody->m_userid&&current_gid == userNobody->m_groupid)
                   {
                       exit(0);
                   }
                   exit(2);
               },
               ::testing::ExitedWithCode(0), ".*");
}

TEST_F(TestSecurityUtils, TestDropPrivilegeLowPrivCanNotDrop) // NOLINT
{
   MAYSKIP;

   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   ASSERT_EXIT({
                   auto userSshd = getUserIdAndGroupId("lp", "lp", m_out);
                   dropPrivileges("lp", "lp", m_out);

                   auto current_uid = getuid();
                   auto current_gid = getgid();
                   if (current_uid == userSshd->m_userid&&current_gid == userSshd->m_groupid)
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

TEST_F(TestSecurityUtils, TestSetupJailAndGoInSetsPwdEnvVar) // NOLINT
{
   MAYSKIP;
   ::testing::FLAGS_gtest_death_test_style = "threadsafe";

   ASSERT_EXIT({
                   char buff[PATH_MAX] = {0};
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

TEST_F(TestSecurityUtils, TestSetupJailAndGoInNoOutsideFileAccess) // NOLINT
{
   MAYSKIP;
   ::testing::FLAGS_gtest_death_test_style = "threadsafe";

   ASSERT_EXIT({
       setupJailAndGoIn("/tmp", m_out);
       //test can't see outside chroot /tmp/tempath*
       std::ifstream passwdFile ("/etc/passwd");
       if (!passwdFile.is_open())
       {
           exit(0);
       }
       exit(2);  },
               ::testing::ExitedWithCode(0),".*");
}

TEST_F(TestSecurityUtils, TestSetupJailAndGoInCanCreateFilesInNewRoot) // NOLINT
{
   MAYSKIP;
   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   Tests::TempDir tempDir("/tmp");
   std::string fileINewRoot("testNewRoot.txt");

   ASSERT_EXIT({
       std::string chrootdir = tempDir.dirPath();

       setupJailAndGoIn(chrootdir, m_out);

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


TEST_F(TestSecurityUtils, TestSetupJailAndGoInAbortsOnFailure) // NOLINT
{
   MAYSKIP;
   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   ASSERT_EXIT({
       std::string chrootdir = ("/notpath/");
       try{
            setupJailAndGoIn(chrootdir, m_out);
       }catch( FatalSecuritySetupFailureException&)
       {
           exit(3); 
       }
       
       exit(0);
       },
               ::testing::ExitedWithCode(3),".*");
}

TEST_F(TestSecurityUtils, TestchrootAndDropPrivilegesAbortIfNotRealUser) // NOLINT
{
   MAYSKIP;
   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   ASSERT_EXIT({
                    try{
                        chrootAndDropPrivileges("notArealUser", "notArealGrp", "/tmp", m_out);
                    }catch(FatalSecuritySetupFailureException& fsex)
                    {
                        exit(3); 
                    }
                   
                   exit(0);
               },
               ::testing::ExitedWithCode(3), ".*");
}

TEST_F(TestSecurityUtils, TestChrootAndDropPrivilegesSuccessfully) // NOLINT
{
   MAYSKIP;

   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   ASSERT_EXIT({
                   auto userNobody = getUserIdAndGroupId("lp", "lp", m_out);
                   auto nobodyUid = userNobody->m_userid;
                   auto nobodyGid = userNobody->m_groupid;

                   chrootAndDropPrivileges("lp", "lp", "/tmp", m_out);

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


class TestSecurityUtilsBindMount : public ::testing::Test
{

public:
    TestSecurityUtilsBindMount() : m_rootPath("/tmp"), m_tempDir{m_rootPath, "TestSecurityUtils"}
    {
        testing::FLAGS_gtest_death_test_style = "threadsafe";
    }
    ~TestSecurityUtilsBindMount()
    {
        unMount(m_targetDir, m_output);
        unMount(m_targetFile, m_output);
    }

    std::vector<std::pair<std::string,int>> m_output;
    std::string m_rootPath;
    Tests::TempDir m_tempDir;
    std::string m_sourceDir;
    std::string m_targetDir;
    std::string m_targetFile; 


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

TEST_F(TestSecurityUtilsBindMount, TestBindMountReadOnlyMountDir) // NOLINT
{
    MAYSKIP;

    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "test");    

    bindMountReadOnly(m_sourceDir, m_targetDir, m_output);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    EXPECT_EQ(Common::FileSystem::fileSystem()->readFile(testFileTarget), "test"); 

    // files can not be dropped in 'read-only' mounted dirs. 
    auto dropFileTarget = Common::FileSystem::join(m_targetDir, "tryThisFile.txt");        
    EXPECT_THROW(Common::FileSystem::fileSystem()->writeFile(dropFileTarget,"thisWillFail"), Common::FileSystem::IFileSystemException);

    unMount(m_targetDir, m_output);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(Common::FileSystem::join(m_targetDir, "SPL.NOTMOUNTED_MARKER")));
}

TEST_F(TestSecurityUtilsBindMount, TestBindMountReadOnlyMountFile) // NOLINT
{
    MAYSKIP;
    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "source");

    //Target will be marked with content SPL.GL_NOTMOUNTED_MARKER
    m_tempDir.createFile(testFileTarget, "");
    bindMountReadOnly(testFileSrc, testFileTarget, m_output);
    auto afterBindMount = Common::FileSystem::fileSystem()->readFile(testFileTarget);
    ASSERT_STREQ(afterBindMount.c_str(), "source");

    unMount(testFileTarget, m_output);
    auto afterUnmount = Common::FileSystem::fileSystem()->readFile(testFileTarget);
    ASSERT_STREQ(afterUnmount.c_str(), "SPL.NOTMOUNTED_MARKER");
}

TEST_F(TestSecurityUtilsBindMount, WillNotMountIfAlreadyMounted) // NOLINT
{
    MAYSKIP;

    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "test");
    bindMountReadOnly(m_sourceDir, m_targetDir, m_output);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    //This will not run mount otherwise we would require to unmount twice
    bindMountReadOnly(m_sourceDir, m_targetDir, m_output);

    //check only a single unmount if required
    unMount(m_targetDir, m_output);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
}

TEST_F(TestSecurityUtilsBindMount, WillUsePreviousilyCreatedMountTargets) // NOLINT
{
    MAYSKIP;

    setupAfterSkipIfNotRoot();
    auto testFileSrc = Common::FileSystem::join(m_sourceDir, "testMountDir.txt");
    auto testFileTarget = Common::FileSystem::join(m_targetDir, "testMountDir.txt");
    m_tempDir.createFile(testFileSrc, "test");

    bindMountReadOnly(m_sourceDir, m_targetDir, m_output);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));

    //check only a single unmount if required
    unMount(m_targetDir, m_output);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(Common::FileSystem::join(m_targetDir, "SPL.NOTMOUNTED_MARKER")));

    //Will mount over previously created file
    bindMountReadOnly(m_sourceDir, m_targetDir, m_output);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));

    unMount(m_targetDir, m_output);
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
}