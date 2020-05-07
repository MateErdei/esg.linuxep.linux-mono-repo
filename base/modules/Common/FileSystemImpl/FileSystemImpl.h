/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/FileSystem/IFileSystem.h"

namespace Common
{
    namespace FileSystem
    {
        class FileSystemImpl : public IFileSystem
        {
        public:
            FileSystemImpl() = default;

            void copyFile(const Path& src, const Path& dest) const override;

            void copyFileAndSetPermissions(
                const Path& src,
                const Path& dest,
                mode_t mode,
                const std::string& ownerName,
                const std::string& groupName) const override;

            bool exists(const Path& path) const override;

            bool isExecutable(const Path& path) const override;

            bool isFile(const Path& path) const override;

            bool isDirectory(const Path& path) const override;

            bool isFileOrDirectory(const Path& path) const override;

            bool isSymlink(const Path& path) const override;

            Path makeAbsolute(const Path& path) const override;

            Path currentWorkingDirectory() const override;

            std::vector<Path> listFilesAndDirectories(const Path& directoryPath, bool includeSymlinks = false)
                const override;

            std::vector<Path> listDirectories(const Path& directoryPath) const override;

            void moveFile(const Path& sourcePath, const Path& destPath) const override;

            std::string readFile(const Path& path) const override;

            void writeFile(const Path& path, const std::string& content) const override;

            void writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir) const override;

            void makeExecutable(const Path& path) const override;

            void makedirs(const Path& path) const override;

            std::vector<Path> listFiles(const Path& directoryPath) const override;

            std::string readFile(const Path& path, unsigned long maxSize) const override;

            std::vector<std::string> readLines(const Path& path) const override;

            std::vector<std::string> readLines(const Path& path, unsigned long maxSize) const override;

            void removeFile(const Path& path) const override;

            void removeFileOrDirectory(const Path& path) const override;

            Path readlink(const Path& path) const override;

            off_t fileSize(const Path& path) const override;

            std::time_t lastModifiedTime(const Path& path) const override;

            void removeFilesInDirectory(const Path& path) const override;
        };

        std::unique_ptr<IFileSystem>& fileSystemStaticPointer();

    } // namespace FileSystem
} // namespace Common
