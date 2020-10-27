/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace Common::FileSystem;

class MockFileSystem : public Common::FileSystem::IFileSystem
{
public:
    MockFileSystem(bool initApplicationConfiguration = true)
    {
        if (initApplicationConfiguration)
        {
            // Initialise the application configuration in the constructor of the mock to avoid
            // hitting a unexpected readlink when performing a strict mock
            Common::ApplicationConfiguration::applicationConfiguration();
        }
    }
    MOCK_CONST_METHOD1(exists, bool(const Path& path));
    MOCK_CONST_METHOD1(isExecutable, bool(const Path& path));
    MOCK_CONST_METHOD1(isFile, bool(const Path& path));
    MOCK_CONST_METHOD1(isDirectory, bool(const Path& path));
    MOCK_CONST_METHOD1(isFileOrDirectory, bool(const Path& path));
    MOCK_CONST_METHOD1(isSymlink, bool(const Path& path));
    MOCK_CONST_METHOD0(currentWorkingDirectory, Path());
    MOCK_CONST_METHOD2(moveFile, void(const Path& sourcePath, const Path& destPath));
    MOCK_CONST_METHOD1(readFile, std::string(const Path& path));
    MOCK_CONST_METHOD2(readFile, std::string(const Path& path, unsigned long maxSize));
    MOCK_CONST_METHOD1(readLines, std::vector<std::string>(const Path& path));
    MOCK_CONST_METHOD2(readLines, std::vector<std::string>(const Path& path, unsigned long maxSize));
    MOCK_CONST_METHOD2(writeFile, void(const Path& path, const std::string& content));
    MOCK_CONST_METHOD3(writeFileAtomically, void(const Path& path, const std::string& content, const Path& tempDir));
    MOCK_CONST_METHOD4(writeFileAtomically, void(const Path& path, const std::string& content, const Path& tempDir, mode_t mode));
    MOCK_CONST_METHOD1(makeExecutable, void(const Path& path));
    MOCK_CONST_METHOD1(listFiles, std::vector<Path>(const Path& directoryPath));
    MOCK_CONST_METHOD2(listFilesAndDirectories, std::vector<Path>(const Path& directoryPath, bool includeSymlinks));
    MOCK_CONST_METHOD1(listDirectories, std::vector<Path>(const Path& directoryPath));
    MOCK_CONST_METHOD1(removeFile, void(const Path& path));
    MOCK_CONST_METHOD1(removeFileOrDirectory, void(const Path& path));
    MOCK_CONST_METHOD2(copyFile, void(const Path& src, const Path& dest));
    MOCK_CONST_METHOD5(copyFileAndSetPermissions, void(const Path& src,const Path& dest, const mode_t mode, const std::string& ownerName, const std::string& groupName));
    MOCK_CONST_METHOD1(makedirs, void(const Path& path));
    MOCK_CONST_METHOD1(makeAbsolute, Path(const Path& path));
    MOCK_CONST_METHOD1(readlink, Path(const Path& path));
    MOCK_CONST_METHOD1(fileSize, off_t(const Path& path));
    MOCK_CONST_METHOD1(lastModifiedTime, std::time_t(const Path& path));
    MOCK_CONST_METHOD1(removeFilesInDirectory, void(const Path& path));
    MOCK_CONST_METHOD2(readProcFile, std::optional<std::string>(int pid, const std::string& filename));
};
