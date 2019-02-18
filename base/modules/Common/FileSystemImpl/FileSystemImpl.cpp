/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystemImpl.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFileTooLargeException.h>
#include <Common/FileSystem/IPermissionDeniedException.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cassert>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

#define LOGSUPPORT(x) std::cout << x << "\n"; // NOLINT

namespace
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
    bool isReaddirSafe(const std::string& directoryPath)
    {
        auto nameMax = pathconf(directoryPath.c_str(), _PC_NAME_MAX);
        struct dirent structdirent;
        return nameMax <= 255 && sizeof(structdirent) > 256;
    }
#pragma GCC diagnostic pop
} // namespace

namespace Common
{
    namespace FileSystem
    {
        Path join(const Path& path1, const Path& path2)
        {
            if (path2.find('/') == 0)
            {
                // If path2 is absolute then use that only
                return path2;
            }

            std::string subPath2;

            if (path2.find("./") == 0)
            {
                subPath2 = path2.substr(2);
            }
            else
            {
                subPath2 = path2;
            }

            if (path1.empty())
            {
                return subPath2;
            }

            if (subPath2.empty())
            {
                return path1;
            }

            if (path1.back() != '/' && subPath2.front() != '/')
            {
                return path1 + '/' + subPath2;
            }

            if ((path1.back() != '/' && subPath2.front() == '/') || (path1.back() == '/' && subPath2.front() != '/'))
            {
                return path1 + subPath2;
            }

            if (path1.back() == '/' && subPath2.front() == '/')
            {
                return path1 + subPath2.substr(1);
            }

            return "";
        }

        Path join(const Path& path1, const Path& path2, const Path& path3) { return join(join(path1, path2), path3); }

        std::string basename(const Path& path)
        {
            size_t pos = path.rfind('/');

            if (pos == Path::npos)
            {
                return path;
            }
            return path.substr(pos + 1);
        }

        std::string dirName(const Path& path)
        {
            std::string tempPath;
            if (path.back() == '/')
            {
                tempPath = std::string(path.begin(), path.end() - 1);
            }
            else
            {
                tempPath = path;
            }

            size_t pos = tempPath.rfind('/');

            if (pos == Path::npos)
            {
                return ""; // no parent directory
            }

            size_t endPos = path.length() - pos;

            return Path(path.begin(), (path.end() - endPos));
        }

        bool FileSystemImpl::exists(const Path& path) const
        {
            struct stat statbuf; // NOLINT
            int ret = stat(path.c_str(), &statbuf);
            return ret == 0;
        }

        bool FileSystemImpl::isFile(const Path& path) const
        {
            struct stat statbuf;
            int ret = stat(path.c_str(), &statbuf);
            if (ret != 0)
            { // if it does not exists, it is not a file
                return false;
            }
            return S_ISREG(statbuf.st_mode);
        }

        bool FileSystemImpl::isDirectory(const Path& path) const
        {
            struct stat statbuf; // NOLINT
            int ret = stat(path.c_str(), &statbuf);
            if (ret != 0)
            { // if it does not exists, it is not a directory
                return false;
            }
            return S_ISDIR(statbuf.st_mode); // NOLINT
        }

        Path FileSystemImpl::currentWorkingDirectory() const
        {
            char currentWorkingDirectory[FILENAME_MAX];

            if (!getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)))
            {
                int err = errno;
                std::string errdesc = ::strerror(err);

                throw IFileSystemException(errdesc);
            }

            return Path(currentWorkingDirectory);
        }

        void FileSystemImpl::moveFile(const Path& sourcePath, const Path& destPath) const
        {
            if (::rename(sourcePath.c_str(), destPath.c_str()) != 0)
            {
                int err = errno;
                std::stringstream errorStream;
                errorStream << "Could not move " << sourcePath << " to " << destPath << ": " << ::strerror(err);

                throw IFileSystemException(errorStream.str());
            }
        }

        std::string FileSystemImpl::readFile(const Path& path) const { return readFile(path, 1024 * 1024 * 10); }

        std::string FileSystemImpl::readFile(const Path& path, unsigned long maxSize) const
        {
            if (isDirectory(path))
            {
                throw IFileSystemException("Error, Failed to read file: '" + path + "', is a directory");
            }

            std::ifstream inFileStream(path, std::ios::binary);

            if (!inFileStream.good())
            {
                throw IFileSystemException("Error, Failed to read file: '" + path + "', file does not exist");
            }

            try
            {
                inFileStream.seekg(0, std::istream::end);
                std::ifstream::pos_type size(inFileStream.tellg());

                if (size < 0)
                {
                    throw IFileSystemException("Error, Failed to read file: '" + path + "', failed to get file size");
                }
                else if (static_cast<unsigned long>(size) > maxSize)
                {
                    throw IFileTooLargeException("Error, Failed to read file: '" + path + "', file too large");
                }

                inFileStream.seekg(0, std::istream::beg);

                std::string content(static_cast<std::size_t>(size), 0);
                inFileStream.read(&content[0], size);
                return content;
            }
            catch (std::system_error& ex)
            {
                LOGSUPPORT(ex.what());
                throw IFileSystemException(std::string("Error, Failed to read from file '") + path + "'");
            }
        }

        void FileSystemImpl::writeFile(const Path& path, const std::string& content) const
        {
            std::ofstream outFileStream(path.c_str(), std::ios::out);

            if (!outFileStream.good())
            {
                int err = errno;
                std::string errdesc = ::strerror(err);

                throw IFileSystemException("Error, Failed to create file: '" + path + "', " + errdesc);
            }

            outFileStream << content;

            outFileStream.close();
        }

        void FileSystemImpl::writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir)
            const
        {
            if (!isDirectory(tempDir))
            {
                throw IFileSystemException("Temp directory provided is not a directory: " + tempDir);
            }

            // create temp file name.
            std::string tempFilePath = join(tempDir, basename(path) + "_tmp");

            try
            {
                writeFile(tempFilePath, content);

                moveFile(tempFilePath, path);
            }
            catch (IFileSystemException&)
            {
                int ret = ::remove(tempFilePath.c_str());
                // Prefer to throw the original exception rather than anything related to failing to delete the temp
                // file
                static_cast<void>(ret);
                throw;
            }
        }

        bool FileSystemImpl::isExecutable(const Path& path) const
        {
            struct stat statbuf; // NOLINT
            int ret = stat(path.c_str(), &statbuf);
            if (ret != 0)
            { // if it does not exists, it is not executable
                return false;
            }
            return (S_IXUSR & statbuf.st_mode) != 0; // NOLINT
        }

        void FileSystemImpl::makeExecutable(const Path& path) const
        {
            struct stat statbuf; // NOLINT
            int ret = stat(path.c_str(), &statbuf);
            if (ret != 0)
            { // if it does not exist
                throw IFileSystemException("Cannot stat: " + path);
            }

            Common::FileSystem::filePermissions()->chmod(
                path.c_str(), statbuf.st_mode | S_IXUSR | S_IXGRP | S_IXOTH); // NOLINT
        }

        void FileSystemImpl::makedirs(const Path& path) const
        {
            if (path == "/")
            {
                return;
            }
            if (isDirectory(path))
            {
                return;
            }
            Path p2 = dirName(path);
            if (path != p2)
            {
                makedirs(p2);
                int ret = ::mkdir(path.c_str(), 0700);
                if (ret == -1)
                {
                    if (errno == EEXIST)
                    {
                        return;
                    }

                    std::ostringstream ost;
                    ost << "Failed to create directory " << path << " error " << strerror(errno) << " (" << errno
                        << ")";

                    throw IFileSystemException(ost.str());
                }
            }
        }

        void FileSystemImpl::copyFile(const Path& src, const Path& dest) const
        {
            if (!FileSystem::fileSystem()->exists(src))
            {
                throw IFileSystemException(
                    "Failed to copy file: '" + src + "' to '" + dest + "', source file does not exist.");
            }
            {
                std::ifstream ifs(src);
                if (!ifs.good())
                {
                    throw IFileSystemException(
                        "Failed to copy file: '" + src + "' to '" + dest + "', ifstream had errors.");
                }

                std::ofstream ofs(dest);
                ofs << ifs.rdbuf();
            }
            if (!FileSystem::fileSystem()->exists(dest))
            {
                throw IFileSystemException(
                    "Failed to copy file: '" + src + "' to '" + dest + "', dest file was not created.");
            }
        }

        std::vector<Path> FileSystemImpl::listFiles(const Path& directoryPath) const
        {
            std::vector<Path> files;
            DIR* directoryPtr;

            struct dirent* outDirEntity;

            directoryPtr = opendir(directoryPath.c_str());

            if (!directoryPtr)
            {
                int error = errno;
                std::string reason = strerror(error);
                throw IFileSystemException("Failed to read directory: '" + directoryPath + "', error:  " + reason);
            }

            assert(isReaddirSafe(directoryPath));

            while (true)
            {
                errno = 0;
                outDirEntity = readdir(directoryPtr);

                if (errno != 0 || outDirEntity == nullptr)
                {
                    break;
                }

                if (DT_REG & outDirEntity->d_type)
                {
                    std::string fullPath = join(directoryPath, outDirEntity->d_name);
                    files.push_back(fullPath);
                }
            }

            (void)closedir(directoryPtr);

            return files;
        }

        std::vector<Path> FileSystemImpl::listFilesAndDirectories(const Path& directoryPath) const
        {
            static std::string dot{ "." };
            static std::string dotdot{ ".." };
            std::vector<Path> files;
            DIR* directoryPtr;

            struct dirent* outDirEntity;

            directoryPtr = opendir(directoryPath.c_str());

            if (!directoryPtr)
            {
                int error = errno;
                std::string reason = strerror(error);
                throw IFileSystemException("Failed to read directory: '" + directoryPath + "', error:  " + reason);
            }

            assert(isReaddirSafe(directoryPath));

            while (true)
            {
                errno = 0;
                outDirEntity = readdir(directoryPtr);

                if (errno != 0 || outDirEntity == nullptr)
                {
                    break;
                }

                if ((DT_REG | DT_DIR) & outDirEntity->d_type && outDirEntity->d_name != dot &&
                    outDirEntity->d_name != dotdot)
                {
                    std::string fullPath = join(directoryPath, outDirEntity->d_name);

                    // we do not want to return symlinks as it could create a infinite loop if the caller calls this
                    // method again on the returned directories
                    struct stat buf;
                    int ret = ::lstat(fullPath.c_str(), &buf);

                    if (ret == 0 && !S_ISLNK(buf.st_mode))
                    {
                        files.push_back(fullPath);
                    }
                }
            }

            (void)closedir(directoryPtr);

            return files;
        }

        void FileSystemImpl::removeFile(const Path& path) const
        {
            if (::remove(path.c_str()) != 0)
            {
                int errn = errno;
                std::string error_cause = ::strerror(errn);
                throw Common::FileSystem::IFileSystemException(
                    "Failed to delete file: " + path + ". Cause: " + error_cause);
            }
        }

        Path FileSystemImpl::make_absolute(const Path& path) const
        {
            if (path[0] == '/')
            {
                return path;
            }

            return Common::FileSystem::join(currentWorkingDirectory(), path);
        }

        std::unique_ptr<Common::FileSystem::IFileSystem>& fileSystemStaticPointer()
        {
            static std::unique_ptr<Common::FileSystem::IFileSystem> instance =
                std::unique_ptr<Common::FileSystem::IFileSystem>(new Common::FileSystem::FileSystemImpl());
            return instance;
        }
    } // namespace FileSystem
} // namespace Common

Common::FileSystem::IFileSystem* Common::FileSystem::fileSystem()
{
    return Common::FileSystem::fileSystemStaticPointer().get();
}
