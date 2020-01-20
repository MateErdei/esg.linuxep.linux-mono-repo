/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <modules/pluginimpl/Logger.h>

#include <modules/pluginimpl/OsqueryDataManager.h>
#include <Common/Helpers/TempDir.h>

using namespace Common::FileSystem;

TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto* ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}

TEST(TestLinkerWorks, WorksWithTheLogging) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    LOGINFO("Produce this logging");
}

TEST(OsqueryDataManager, rotateFileWorks)
{
    OsqueryDataManager m_DataManager;
    auto* ifileSystem = Common::FileSystem::fileSystem();
    Tests::TempDir tempdir{"/tmp"};

    std::string basefile = tempdir.dirPath()+"/file";


    ifileSystem->writeFileAtomically(basefile + ".1","blah1","/tmp");
    ifileSystem->writeFileAtomically(basefile +".2","blah2","/tmp");
    m_DataManager.rotateFiles(basefile,2);

    ASSERT_TRUE(ifileSystem->isFile(basefile + ".2"));
    ASSERT_EQ(ifileSystem->readFile(basefile + ".2"),"blah1");
    ASSERT_TRUE(ifileSystem->isFile(basefile + ".3"));
    ASSERT_EQ(ifileSystem->readFile(basefile + ".3"),"blah2");
}