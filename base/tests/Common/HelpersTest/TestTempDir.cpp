/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/Helpers/TempDir.h>

#include "gtest/gtest.h"

namespace
{
using namespace Tests;
using namespace Common::FileSystem;
    TEST( TestTempDir, ConstructorCreatesTheTemporaryDirectory)
    {
        auto fileSystem = Common::FileSystem::FileSystemImpl();
        TempDir td;
        ASSERT_TRUE( fileSystem.isDirectory(td.dirPath()));
    }

    TEST( TestTempDir, DestructorRemovesTheTemporaryDirectory)
    {

        auto fileSystem = Common::FileSystem::FileSystemImpl();
        auto td = TempDir::makeTempDir("destructor");
        std::string dirPath = td->dirPath();
        ASSERT_TRUE( fileSystem.isDirectory(dirPath));
        td.reset(nullptr);
        EXPECT_FALSE(fileSystem.isDirectory(dirPath));
    }

    TEST( TestTempDir, AbsolutePaths)
    {
        auto fileSystem = Common::FileSystem::FileSystemImpl();
        TempDir td;
        std::string dirPath = td.dirPath();
        EXPECT_TRUE( fileSystem.isDirectory(dirPath));
        EXPECT_TRUE( dirPath.front() == '/');
        std::string relativePath = "level1/level2";
        std::string absPath = td.absPath(relativePath);
        EXPECT_EQ( absPath.substr(0,dirPath.size()), dirPath);
        EXPECT_EQ( absPath.substr(dirPath.size()+1,absPath.size()-dirPath.size()-1), relativePath);
    }

    TEST( TestTempDir, MakeDirs)
    {
        auto fileSystem = Common::FileSystem::FileSystemImpl();
        TempDir td;
        std::string dirPath = td.dirPath();
        std::string relativePath = "level1/level2/level3";
        td.makeDirs(relativePath);
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/level1"));
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/level1/level2"));
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/level1/level2/level3"));
    }


    TEST( TestTempDir, MakeDirsFromVector)
    {
        std::vector<std::string> relatives = {{"a/b/c"}, {"a/b/d"}, {"a/c/d/"}};
        auto fileSystem = Common::FileSystem::FileSystemImpl();
        TempDir td;
        std::string dirPath = td.dirPath();
        td.makeDirs(relatives);
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/a"));
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/a/b/c"));
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/a/b/d"));
        ASSERT_TRUE( fileSystem.isDirectory(dirPath+"/a/c/d"));
    }


    TEST( TestTempDir, createFile)
    {
        auto fileSystem = Common::FileSystem::FileSystemImpl();
        TempDir td;
        std::string dirPath = td.dirPath();
        std::string expected_content= "hello world";
        td.createFile("dirname/filename.txt", expected_content);
        std::string content = fileSystem.readFile(dirPath+"/dirname/filename.txt");
        EXPECT_EQ(content, "hello world");

        td.createFile("filename.txt", expected_content);
        content = fileSystem.readFile(dirPath+"/filename.txt");
        EXPECT_EQ(content, "hello world");



    }

}