/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>

#include "TempDirBaseClassTest.h"

/**
 * This class is to show how TempDirBaseClassTest can be used in order to
 * have a temporary directory for ever single tests.
 */
class UsageExample : public Tests::TempDirBaseClassTest
{
public:
    std::string myTest;

    /**
     * Developers do not need to override this method, but if they do, they must remember to
     * call Tests::TempDirBaseClassTest::SetUp();
     */
    void SetUp() override
    {
        // execute the setup of base class
        Tests::TempDirBaseClassTest::SetUp();
        // do your stuff here
        myTest = "UsageExample";
    }

    /**
     * Developers do not need to override this method, but if they do, they must remember to call
     * Tests::TempDirBaseClassTest::TearDown();
     */
    void TearDown() override
    {
        // do your stuff here
        myTest.clear();
        // call the TearDown of base class.
        Tests::TempDirBaseClassTest::TearDown();
    }
};

// show that directories are created for each test and the tests are completely independent from each other.

TEST_F( UsageExample, FreshDirectoryIsCreated)
{
    auto fileSystem = Common::FileSystem::FileSystemImpl();
    ASSERT_TRUE( fileSystem.isDirectory(m_tempDir->dirPath()));
    m_tempDir->createFile("test1.txt", "content");
    ASSERT_FALSE(fileSystem.exists(m_tempDir->absPath("test2.txt")));
    ASSERT_TRUE(fileSystem.exists(m_tempDir->absPath("test1.txt")));
    ASSERT_EQ(myTest, "UsageExample");
}

TEST_F( UsageExample, TestDirectoryIsIndependent)
{
    auto fileSystem = Common::FileSystem::FileSystemImpl();
    ASSERT_TRUE( fileSystem.isDirectory(m_tempDir->dirPath()));
    m_tempDir->createFile("test2.txt", "content");
    ASSERT_FALSE(fileSystem.exists(m_tempDir->absPath("test1.txt")));
    ASSERT_TRUE(fileSystem.exists(m_tempDir->absPath("test2.txt")));
}