/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/TempDir.h>

#include <modules/pluginimpl/OsqueryDataManager.h>


#include <gtest/gtest.h>

TEST(TestOsqueryDataManager, rotateFileWorks)
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