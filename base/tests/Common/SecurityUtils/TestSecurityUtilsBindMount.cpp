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


class TestSecurityUtilsBindMount : public ::testing::Test
{

public:
    TestSecurityUtilsBindMount() : m_rootPath("/tmp"), m_tempDir{m_rootPath, "TestSecurityUtils"}
    {
        GTEST_FLAG_SET(death_test_style, "threadsafe");
    }
    ~TestSecurityUtilsBindMount() override
    {
        unMount(m_targetDir, m_output);
        unMount(m_targetFile, m_output);
    }

    std::string getOutput()
    {
        std::ostringstream ost;
        for (const auto& [line, level] : m_output)
        {
            ost << line << '\n';
        }
        return ost.str();
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

TEST_F(TestSecurityUtilsBindMount, TestBindMountReadOnlyMountDir)
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

    auto ret = unMount(m_targetDir, m_output);
    EXPECT_TRUE(ret) << "unMount failed: " << getOutput();
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget)) << "unMount silently failed: " << getOutput();
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(Common::FileSystem::join(m_targetDir, "SPL.NOTMOUNTED_MARKER")))
        << "unMount silently failed: " << getOutput();
}

TEST_F(TestSecurityUtilsBindMount, TestBindMountReadOnlyMountFile)
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

    auto ret = unMount(testFileTarget, m_output);
    EXPECT_TRUE(ret) << "unMount failed: " << getOutput();
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
    auto ret = unMount(m_targetDir, m_output);
    EXPECT_TRUE(ret) << "unMount failed: " << getOutput();
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
    auto ret = unMount(m_targetDir, m_output);
    EXPECT_TRUE(ret) << "unMount failed: " << getOutput();
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(Common::FileSystem::join(m_targetDir, "SPL.NOTMOUNTED_MARKER")));

    //Will mount over previously created file
    bindMountReadOnly(m_sourceDir, m_targetDir, m_output);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->exists(testFileTarget));

    ret = unMount(m_targetDir, m_output);
    EXPECT_TRUE(ret) << "unMount failed: " << getOutput();
    ASSERT_FALSE(Common::FileSystem::fileSystem()->exists(testFileTarget));
}