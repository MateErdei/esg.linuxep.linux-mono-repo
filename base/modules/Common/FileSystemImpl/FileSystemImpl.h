// Copyright 2018-2022, Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IFileSystem.h"

namespace Common::FileSystem
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
        std::vector<Path> listAllFilesInDirectoryTree(const Path& root) const override;
        std::vector<Path> listAllFilesAndDirsInDirectoryTree(const Path& root) const override;
        std::vector<Path> listDirectories(const Path& directoryPath) const override;

        void moveFileTryCopy(const Path& sourcePath, const Path& destPath) const override;

        void moveFile(const Path& sourcePath, const Path& destPath) const override;

        std::string readFile(const Path& path) const override;

        void appendFile(const Path& path, const std::string& content) const override;

        void writeFile(const Path& path, const std::string& content) const override;

        void writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir, mode_t mode)
            const override;

        void writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir) const override;

        void makeExecutable(const Path& path) const override;

        void makedirs(const Path& path) const override;

        std::vector<Path> listFiles(const Path& directoryPath) const override;

        std::string readFile(const Path& path, unsigned long maxSize) const override;

        std::vector<std::string> readLines(const Path& path) const override;

        std::vector<std::string> readLines(const Path& path, unsigned long maxSize) const override;

        std::ifstream openFileForRead(const Path& path) const override;

        void removeFile(const Path& path, bool ignoreAbsent) const override;

        void removeFile(const Path& path) const override;

        void removeFileOrDirectory(const Path& path) const override;

        std::optional<Path> readlink(const Path& path) const override;

        off_t fileSize(const Path& path) const override;

        std::time_t lastModifiedTime(const Path& path) const override;

        int getFileInfoDescriptor(const Path& directory) const override;

        int getFileInfoDescriptorFromDirectoryFD(int fd, const Path& path) const override;

        void unlinkFileUsingDirectoryFD(int fd, const Path& path) const override;

        void unlinkDirUsingDirectoryFD(int fd, const Path& path) const override;

        bool compareFileDescriptors(int fd1, int fd2) const override;

        std::optional<std::string> readProcFile(int pid, const std::string& filename) const override;

        void removeFilesInDirectory(const Path& path) const override;

        bool waitForFile(const Path& path, unsigned int timeout) const override;

        void recursivelyDeleteContentsOfDirectory(const Path& path) const override;

        [[nodiscard]] std::string calculateDigest(SslImpl::Digest digestName, const Path& path) const override;

        [[nodiscard]] std::string calculateDigest(SslImpl::Digest digestName, int fd) const override;

        [[nodiscard]] std::filesystem::space_info getDiskSpaceInfo(const Path& path) const override;

        [[nodiscard]] std::filesystem::space_info getDiskSpaceInfo(const Path& path, std::error_code& ec)
            const override;
        std::string getSystemCommandExecutablePath(const std::string& executableName) const override;

    protected:
        virtual int moveFileImpl(const Path& sourcePath, const Path& destPath) const;

    private:
        void walkDirectoryTree(std::vector<Path>& pathCollection, const Path& root) const;
        void throwFileSystemException(const int err, const Path& source, const Path& dest) const;
    };

    std::unique_ptr<IFileSystem>& fileSystemStaticPointer();
} // namespace Common::FileSystem
