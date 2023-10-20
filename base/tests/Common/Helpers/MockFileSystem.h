// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"

#include <gmock/gmock.h>
#include <fstream>
using namespace ::testing;
using namespace Common::FileSystem;

/**
 * Mock IFileSystem
 * This can't be in anonymous namespace because it is exposed to Plugins
 * which have member variables of MockFileSystem* type.
 */
class MockFileSystem : public Common::FileSystem::IFileSystem
{
public:
    explicit MockFileSystem(bool initApplicationConfiguration = true)
    {
        if (initApplicationConfiguration)
        {
            // Initialise the application configuration in the constructor of the mock to avoid
            // hitting an unexpected readlink when performing a strict mock
            std::ignore = Common::ApplicationConfiguration::applicationConfiguration();
        }
    }
    MOCK_METHOD(bool, exists, (const Path& path), (const, override));
    MOCK_METHOD(bool, isExecutable, (const Path& path), (const, override));
    MOCK_METHOD(bool, isFile, (const Path& path), (const, override));
    MOCK_METHOD(bool, isDirectory, (const Path& path), (const, override));
    MOCK_METHOD(bool, isFileOrDirectory, (const Path& path), (const, override));
    MOCK_METHOD(bool, isSymlink, (const Path& path), (const, override));
    MOCK_METHOD(Path, currentWorkingDirectory, (), (const, override));
    MOCK_METHOD(void, moveFile, (const Path& sourcePath, const Path& destPath), (const, override));
    MOCK_METHOD(void, moveFileTryCopy, (const Path& sourcePath, const Path& destPath), (const, override));
    MOCK_METHOD(std::string, readFile, (const Path& path), (const, override));
    MOCK_METHOD(std::string, readFile, (const Path& path, unsigned long maxSize), (const, override));
    MOCK_METHOD((std::vector<std::string>), readLines, (const Path& path), (const, override));
    MOCK_METHOD((std::vector<std::string>), readLines, (const Path& path, unsigned long maxSize), (const, override));
    MOCK_METHOD((std::ifstream), openFileForRead, (const Path& path), (const, override));
    MOCK_METHOD(void, appendFile, (const Path& path, const std::string& content), (const, override));
    MOCK_METHOD(void, writeFile, (const Path& path, const std::string& content), (const, override));
    MOCK_METHOD(void, writeFileAtomically, (const Path& path, const std::string& content, const Path& tempDir), (const, override));
    MOCK_METHOD(void, writeFileAtomically, (const Path& path, const std::string& content, const Path& tempDir, mode_t mode), (const, override));
    MOCK_METHOD(void, makeExecutable, (const Path& path), (const, override));
    MOCK_METHOD((std::vector<Path>), listFiles, (const Path& directoryPath), (const, override));
    MOCK_METHOD((std::vector<Path>), listFilesAndDirectories, (const Path& directoryPath, bool includeSymlinks), (const, override));
    MOCK_METHOD((std::vector<Path>), listAllFilesAndDirsInDirectoryTree, (const Path& directoryPath), (const, override));
    MOCK_METHOD((std::vector<Path>), listDirectories, (const Path& directoryPath), (const, override));
    MOCK_METHOD((std::vector<Path>), listAllFilesInDirectoryTree, (const Path& directoryPath), (const, override));
    MOCK_METHOD(void, removeFile, (const Path& path, bool), (const, override));
    MOCK_METHOD(void, removeFile, (const Path& path), (const, override));
    MOCK_METHOD(void, removeFileOrDirectory, (const Path& path), (const, override));
    MOCK_METHOD(void, copyFile, (const Path& src, const Path& dest), (const, override));
    MOCK_METHOD(void, copyFileAndSetPermissions, (const Path& src, const Path& dest, const mode_t mode, const std::string& ownerName, const std::string& groupName), (const, override));
    MOCK_METHOD(void, makedirs, (const Path& path), (const, override));
    MOCK_METHOD(Path, makeAbsolute, (const Path& path), (const, override));
    MOCK_METHOD((std::optional<Path>), readlink, (const Path& path), (const, override));
    MOCK_METHOD(off_t, fileSize, (const Path& path), (const, override));
    MOCK_METHOD(std::time_t, lastModifiedTime, (const Path& path), (const, override));
    MOCK_METHOD(int, getFileInfoDescriptor, (const Path& path), (const, override));
    MOCK_METHOD(int, getFileInfoDescriptorFromDirectoryFD, (int fd1, const Path& path), (const, override));
    MOCK_METHOD(bool, compareFileDescriptors, (int fd1, int fd2), (const, override));
    MOCK_METHOD(void, unlinkFileUsingDirectoryFD, (int fd1, const Path& path), (const, override));
    MOCK_METHOD(void, unlinkDirUsingDirectoryFD, (int fd1, const Path& path), (const, override));
    MOCK_METHOD(void, removeFilesInDirectory, (const Path& path), (const, override));
    MOCK_METHOD((std::optional<std::string>), readProcFile, (int pid, const std::string& filename), (const, override));
    MOCK_METHOD(bool, waitForFile, (const Path& path, unsigned int timeout), (const, override));
    MOCK_METHOD(void, recursivelyDeleteContentsOfDirectory, (const Path& path), (const, override));
    MOCK_METHOD(std::string, calculateDigest, (Common::SslImpl::Digest digestName, const Path& path), (const, override));
    MOCK_METHOD(std::string, calculateDigest, (Common::SslImpl::Digest digestName, int fd), (const, override));
    MOCK_METHOD(std::filesystem::space_info, getDiskSpaceInfo, (const Path& path), (const, override));
    MOCK_METHOD(std::filesystem::space_info, getDiskSpaceInfo, (const Path& path, std::error_code& ec), (const, override));
    MOCK_METHOD(std::string, getSystemCommandExecutablePath, (const std::string& path), (const, override));
};
