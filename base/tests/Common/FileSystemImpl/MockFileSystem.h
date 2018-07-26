/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include "Common/FileSystem/IFileSystem.h"
#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FileSystem;

class MockFileSystem: public Common::FileSystem::IFileSystem
{
public:
    MOCK_CONST_METHOD2(join, Path(const Path& path1, const Path & path2));
    MOCK_CONST_METHOD1(basename, std::string(const Path & path));
    MOCK_CONST_METHOD1(dirName, std::string(const Path & path));
    MOCK_CONST_METHOD1(exists, bool(const Path &path));
    MOCK_CONST_METHOD1(isExecutable, bool(const Path &path));
    MOCK_CONST_METHOD1(isFile, bool(const Path & path));
    MOCK_CONST_METHOD1(isDirectory, bool(const Path & path));
    MOCK_CONST_METHOD0(currentWorkingDirectory, Path());
    MOCK_CONST_METHOD2(moveFile, void(const Path &sourcePath, const Path &destPath));
    MOCK_CONST_METHOD1(readFile, std::string(const Path &path));
    MOCK_CONST_METHOD2(writeFile, void(const Path &path, const std::string &content));
    MOCK_CONST_METHOD3(writeFileAtomically, void(const Path &path, const std::string &content, const Path &tempDir));
    MOCK_CONST_METHOD1(makeExecutable, void(const Path &path));
    MOCK_CONST_METHOD1(listFiles, std::vector<Path>( const Path & directoryPath));
    MOCK_CONST_METHOD1(removeFile, void(const Path &path));
    MOCK_CONST_METHOD2(copyFile, void(const Path& src, const Path& dest));
    MOCK_CONST_METHOD1(makedirs, void(const Path &path));
    MOCK_CONST_METHOD3(join, Path(const Path &path1, const Path &path2, const Path &path3));
};


