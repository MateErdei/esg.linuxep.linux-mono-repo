///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "ExampleTempDirSuiteTest.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
using namespace Tests;

// use this static global variable to check which test has run.
// the logic will break if running repeated_test.
static int testRunned = 0;

// show that tests are not completely independent when using the static entry
TEST_F( ExampleTempDirSuiteTest, DirectoryIsSharedAmongTests_1)
{
    testRunned |= 0x01;
    auto fileSystem = Common::FileSystem::FileSystemImpl();
    ASSERT_TRUE( fileSystem.isDirectory(tempDir->dirPath()));
    tempDir->createFile("test1.txt", "content");
    ASSERT_TRUE(fileSystem.exists(tempDir->absPath("test1.txt")));

    if( testRunned & 0x02)
    {
        ASSERT_TRUE(fileSystem.exists(tempDir->absPath("test2.txt")));
    }
    else
    {
        ASSERT_FALSE(fileSystem.exists(tempDir->absPath("test2.txt")));
    }
}

TEST_F( ExampleTempDirSuiteTest, DirectoryIsSharedAmongTests_2)
{
    testRunned != 0x02;
    auto fileSystem = Common::FileSystem::FileSystemImpl();
    ASSERT_TRUE( fileSystem.isDirectory(tempDir->dirPath()));
    tempDir->createFile("test2.txt", "content");
    ASSERT_TRUE(fileSystem.exists(tempDir->absPath("test2.txt")));
    if( testRunned & 0x01)
    {
        ASSERT_TRUE(fileSystem.exists(tempDir->absPath("test1.txt")));
    }
    else
    {
        ASSERT_FALSE(fileSystem.exists(tempDir->absPath("test1.txt")));
    }
}