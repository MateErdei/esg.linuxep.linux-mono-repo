// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "FileSystemImpl.h"

#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/FileSystem/IPermissionDeniedException.h"
#include "Common/SslImpl/Digest.h"
#include "Common/UtilityImpl/StrError.h"

#include <ext/stdio_filebuf.h>
#include <sys/stat.h>

#include <cassert>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
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

namespace Common::FileSystem
{
    using namespace Common::UtilityImpl;
    static constexpr unsigned long GL_MAX_SIZE = 1024 * 1024 * 10;

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

    Path join(const Path& path1, const Path& path2, const Path& path3)
    {
        return join(join(path1, path2), path3);
    }

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

    std::string subdirNameFromPath(const Path& path)
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
            return tempPath; // no parent directory
        }

        return Path(tempPath.begin() + pos + 1, tempPath.end());
    }

    bool FileSystemImpl::exists(const Path& path) const
    {
        struct stat statbuf; // NOLINT
        int ret = stat(path.c_str(), &statbuf);
        return ret == 0;
    }

    bool FileSystemImpl::isFile(const Path& path) const
    {
        struct stat statbuf
        {
        };
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        { // if it does not exists, it is not a file
            return false;
        }
        return S_ISREG(statbuf.st_mode); // NOLINT
    }

    bool FileSystemImpl::isDirectory(const Path& path) const
    {
        struct stat statbuf
        {
        };
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        { // if it does not exists, it is not a directory
            return false;
        }
        return S_ISDIR(statbuf.st_mode); // NOLINT
    }

    bool FileSystemImpl::isFileOrDirectory(const Path& path) const
    {
        struct stat statbuf
        {
        };
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        { // if it does not exists, it is not a file
            return false;
        }
        return S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode); // NOLINT
    }

    bool FileSystemImpl::isSymlink(const Path& path) const
    {
        struct stat statbuf
        {
        };
        int ret = ::lstat(path.c_str(), &statbuf);
        if (ret != 0)
        { // if it does not exists, it is not a directory
            return false;
        }
        return S_ISLNK(statbuf.st_mode); // NOLINT
    }

    Path FileSystemImpl::currentWorkingDirectory() const
    {
        char currentWorkingDirectory[FILENAME_MAX];

        if (!getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)))
        {
            int error = errno;
            std::string errdesc = StrError(error);

            throw IFileSystemException(errdesc);
        }

        return Path(currentWorkingDirectory);
    }

    void FileSystemImpl::moveFileTryCopy(const Path& sourcePath, const Path& destPath) const
    {
        int ret = moveFileImpl(sourcePath, destPath);
        if (ret == EXDEV)
        {
            copyFile(sourcePath, destPath);
            removeFile(sourcePath);
        }
        else if (ret != 0)
        {
            throwFileSystemException(ret, sourcePath, destPath);
        }
    }

    void FileSystemImpl::moveFile(const Path& sourcePath, const Path& destPath) const
    {
        int ret = moveFileImpl(sourcePath.c_str(), destPath.c_str());
        if (ret != 0)
        {
            throwFileSystemException(ret, sourcePath, destPath);
        }
    }

    int FileSystemImpl::moveFileImpl(const Path& sourcePath, const Path& destPath) const
    {
        if (::rename(sourcePath.c_str(), destPath.c_str()) != 0)
        {
            return errno;
        }
        return 0;
    }

    void FileSystemImpl::throwFileSystemException(const int err, const Path& source, const Path& dest) const
    {
        std::string srcExists = exists(source) ? "exists" : "doesn't exist";

        auto destPath  = dest.back() == '/' ? dest : dirName(dest);
        std::string destExists = exists(destPath) ? "exists" : "doesn't exist";

        std::stringstream errorStream;
        errorStream << "Could not move " << source << "(" << srcExists << ")" <<
                                    " to " << dest << "(" << destExists << ")"  <<
                                      ": " << StrError(err) << "(" << err << ")";

        throw IFileSystemException(errorStream.str());
    }

    std::string FileSystemImpl::readFile(const Path& path) const
    {
        return readFile(path, GL_MAX_SIZE);
    }

    std::string FileSystemImpl::readFile(const Path& path, unsigned long maxSize) const
    {
        if (isDirectory(path))
        {
            throw IFileSystemException("Error, Failed to read file: '" + path + "', is a directory");
        }

        std::ifstream inFileStream(path, std::ios::binary);

        if (!inFileStream.good())
        {
            if (errno == ENOENT)
            {
                throw IFileNotFoundException("Error, Failed to read file: '" + path + "', file does not exist");
            }
            else if (errno == EACCES)
            {
                throw IPermissionDeniedException("Error, Failed to read file: '" + path + "', permission denied");
            }
            else
            {
                throw IFileSystemException("Error, Failed to read file: '" + path + "', " + StrError(errno));
            }
        }

        try
        {
            inFileStream.seekg(0, std::istream::end);
            if (!inFileStream.good())
            {
                throw IFileSystemException(
                    "Error, Failed to read file: '" + path +
                    "', seekg failed, state: " + std::to_string(static_cast<int>(inFileStream.rdstate())));
            }

            std::ifstream::pos_type size(inFileStream.tellg());

            if (!inFileStream.good())
            {
                throw IFileSystemException(
                    "Error, Failed to read file: '" + path +
                    "', tellg failed, state: " + std::to_string(static_cast<int>(inFileStream.rdstate())));
            }

            if (size < 0)
            {
                throw IFileSystemException(
                    "Error, Failed to read file: '" + path +
                    "', failed to get file size. Got: " + std::to_string(size));
            }
            else if (static_cast<unsigned long>(size) > maxSize)
            {
                throw IFileTooLargeException(
                    "Error, Failed to read file: '" + path + "', file too large, size: " + std::to_string(size));
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

    std::vector<std::string> FileSystemImpl::readLines(const Path& path) const
    {
        return readLines(path, GL_MAX_SIZE);
    }

    std::vector<std::string> FileSystemImpl::readLines(const Path& path, unsigned long maxSize) const
    {
        std::vector<std::string> list;
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
            std::string str;
            while (std::getline(inFileStream, str))
            {
                list.push_back(str);
            }
            return list;
        }
        catch (const std::bad_alloc& ex)
        {
            LOGSUPPORT(ex.what());
            throw IFileSystemException(
                "Error, Failed to read from file '" + path + "' caused by memory allocation issue.");
        }
        catch (const std::system_error& ex)
        {
            LOGSUPPORT(ex.what());
            throw IFileSystemException("Error, Failed to read from file '" + path + "'");
        }
    }

    std::ifstream FileSystemImpl::openFileForRead(const Path& path) const
    {
        if (isFile(path))
        {
            std::ifstream infile(path.c_str());
            if (infile.is_open() && infile.good())
            {
                return infile;
            }
            throw IFileSystemException("Failed to open " + path);
        }
        if (isDirectory(path))
        {
            throw IFileSystemException(path + " is a directory");
        }
        throw IFileSystemException(path + " is not a file");
    }

    void FileSystemImpl::appendFile(const Path& path, const std::string& content) const
    {
        std::ofstream outFileStream(path.c_str(), std::ios::app);

        if (!outFileStream.good())
        {
            int error = errno;
            std::string errdesc = StrError(error);

            throw IFileSystemException("Error, Failed to append file: '" + path + "', " + errdesc);
        }

        outFileStream << content;

        outFileStream.close();
    }

    void FileSystemImpl::writeFile(const Path& path, const std::string& content) const
    {
        std::ofstream outFileStream(path.c_str(), std::ios::out);

        if (!outFileStream.good())
        {
            int error = errno;
            std::string errdesc = StrError(error);

            throw IFileSystemException("Error, Failed to open file for write: '" + path + "', " + errdesc);
        }

        outFileStream << content;
        outFileStream.close();

        if (!outFileStream.good())
        {
            int error = errno;
            std::string errdesc = StrError(error);

            throw IFileSystemException("Error, Failed to close file after write: '" + path + "', " + errdesc);
        }
    }

    void FileSystemImpl::writeFileAtomically(
        const Path& path,
        const std::string& content,
        const Path& tempDir,
        mode_t mode) const
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
            if (mode != 0)
            {
                Common::FileSystem::filePermissions()->chmod(tempFilePath, mode);
            }
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

    void FileSystemImpl::writeFileAtomically(const Path& path, const std::string& content, const Path& tempDir) const
    {
        return FileSystemImpl::writeFileAtomically(path, content, tempDir, 0);
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
                int error = errno;
                if (error == EEXIST)
                {
                    return;
                }

                std::ostringstream ost;
                ost << "Failed to create directory " << path << " error " << StrError(error) << " (" << error << ")";

                throw IFileSystemException(ost.str());
            }
        }
    }

    void FileSystemImpl::copyFile(const Path& src, const Path& dest) const
    {
        char copyBuffer[65537];
        auto* fileSystem = FileSystem::fileSystem();
        if (!fileSystem->exists(src))
        {
            throw IFileSystemException(
                "Failed to copy file: '" + src + "' to '" + dest + "', source file does not exist.");
        }
        {
            std::ifstream ifs;
            ifs.rdbuf()->pubsetbuf(copyBuffer, sizeof copyBuffer);
            ifs.open(src, std::ios::binary);
            if (!ifs.good())
            {
                throw IFileSystemException(
                    "Failed to copy file: '" + src + "' to '" + dest + "', reading file failed.");
            }

            std::ofstream ofs(dest, std::ios::binary);
            if (!ofs.good())
            {
                throw IFileSystemException(
                    "Failed to copy file: '" + src + "' to '" + dest + "', writing file failed.");
            }

            ofs << ifs.rdbuf();

            if (ifs.peek() != EOF)
            {
                throw IFileSystemException(
                    "Failed to copy file: '" + src + "' to '" + dest +
                    "', failed to complete writing to file, check space available on device.");
            }
        }

        if (fileSystem->fileSize(src) != 0 && fileSystem->fileSize(dest) == 0)
        {
            fileSystem->removeFile(dest);
            throw IFileSystemException(
                "Failed to copy file: '" + src + "' to '" + dest +
                "', contents failed to copy. Check space available on device.");
        }
    }
    void FileSystemImpl::copyFileAndSetPermissions(
        const Path& src,
        const Path& dest,
        mode_t mode,
        const std::string& ownerName,
        const std::string& groupName) const
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        copyFile(src, dest);
        filePermissions->chown(dest, ownerName, groupName);
        filePermissions->chmod(dest, mode);
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
            throw IFileSystemException("Failed to read directory: '" + directoryPath + "', error:  " + StrError(error));
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

            std::string fullPath = join(directoryPath, outDirEntity->d_name);
            if (DT_REG & outDirEntity->d_type || (DT_UNKNOWN == outDirEntity->d_type && isFile(fullPath)))
            {
                files.push_back(fullPath);
            }
        }

        (void)closedir(directoryPtr);

        return files;
    }

    std::vector<Path> FileSystemImpl::listFilesAndDirectories(const Path& directoryPath, bool includeSymlinks) const
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
            std::string reason = StrError(error);
            throw IFileSystemException("Failed to read directory: '" + directoryPath + "', error:  " + reason);
        }

        assert(isReaddirSafe(directoryPath));

        while (true)
        {
            errno = 0;
            outDirEntity = ::readdir(directoryPtr);

            if (errno != 0 || outDirEntity == nullptr)
            {
                break;
            }

            if (outDirEntity->d_name == dot || outDirEntity->d_name == dotdot)
            {
                continue;
            }

            std::string fullPath = join(directoryPath, outDirEntity->d_name);

            // If regular file or directory (info from dirent struct d_type) or if ftype not enabled on
            // filesystem (d_type is always DT_UNKNOWN) we have to use lstat
            if ((DT_REG | DT_DIR) & outDirEntity->d_type || // NOLINT
                (DT_UNKNOWN == outDirEntity->d_type && isFileOrDirectory(fullPath)))
            {
                // We do not want to return symlinks as it could create an infinite loop if the caller calls this
                // method again on the returned directories
                if (!includeSymlinks && isSymlink(fullPath))
                {
                    continue;
                }

                files.push_back(fullPath);
            }
        }

        (void)closedir(directoryPtr);

        return files;
    }

    std::vector<Path> FileSystemImpl::listAllFilesInDirectoryTree(const Path& root) const
    {
        std::vector<Path> pathCollection;
        walkDirectoryTree(pathCollection, root);
        return pathCollection;
    }

    std::vector<Path> FileSystemImpl::listAllFilesAndDirsInDirectoryTree(const Path& root) const
    {
        std::vector<Path> paths;
        if (isDirectory(root))
        {
            for (auto const& dirEntry : std::filesystem::recursive_directory_iterator(root))
            {
                paths.push_back(dirEntry.path());
            }
        }
        return paths;
    }

    void FileSystemImpl::walkDirectoryTree(std::vector<Path>& pathCollection, const Path& root) const
    {
        std::vector<std::string> files;
        try
        {
            files = listFiles(root);
        }
        catch (IFileSystemException& exception)
        {
            std::cout << "Failed to get list of files for :'" << root << "'" << std::endl;
            return;
        }

        for (auto& file : files)
        {
            pathCollection.push_back(file);
        }

        std::vector<std::string> directories;
        try
        {
            directories = listDirectories(root);
        }
        catch (IFileSystemException& exception)
        {
            std::cout << "Failed to get list of directories for :'" << root << "'" << std::endl;
            return;
        }

        for (auto& directory : directories)
        {
            walkDirectoryTree(pathCollection, directory);
        }
    }

    void FileSystemImpl::removeFile(const Path& path, bool ignoreAbsent) const
    {
        if (::remove(path.c_str()) != 0)
        {
            int error = errno;
            if (ignoreAbsent && error == ENOENT)
            {
                return;
            }

            std::string error_cause = StrError(error);
            throw Common::FileSystem::IFileSystemException(
                "Failed to delete file: " + path + ". Cause: " + error_cause);
        }
    }

    void FileSystemImpl::removeFile(const Path& path) const
    {
        removeFile(path, false);
    }

    void FileSystemImpl::removeFileOrDirectory(const Path& dir) const
    {
        if (isDirectory(dir))
        {
            for (const auto& path : listFilesAndDirectories(dir, true))
            {
                removeFileOrDirectory(path);
            }
        }

        removeFile(dir);
    }

    Path FileSystemImpl::makeAbsolute(const Path& path) const
    {
        if (path[0] == '/')
        {
            return path;
        }

        return Common::FileSystem::join(currentWorkingDirectory(), path);
    }

    std::vector<Path> FileSystemImpl::listDirectories(const Path& directoryPath) const
    {
        static std::string dot{ "." };
        static std::string dotdot{ ".." };
        std::vector<Path> dirs;
        DIR* directoryPtr;

        struct dirent* outDirEntity;

        directoryPtr = opendir(directoryPath.c_str());

        if (!directoryPtr)
        {
            int error = errno;
            std::string reason = StrError(error);
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

            if (outDirEntity->d_name == dot || outDirEntity->d_name == dotdot)
            {
                continue;
            }

            std::string fullPath = join(directoryPath, outDirEntity->d_name);
            if (DT_DIR & outDirEntity->d_type || (DT_UNKNOWN == outDirEntity->d_type && isDirectory(fullPath)))
            {
                // We do not want to return symlinks as it could create an infinite loop if the caller calls this
                // method again on the returned directories
                if (isSymlink(fullPath))
                {
                    continue;
                }
                dirs.push_back(fullPath);
            }
        }

        (void)closedir(directoryPtr);

        return dirs;
    }

    std::optional<Path> FileSystemImpl::readlink(const Path& path) const
    {
        char linkPath[PATH_MAX + 1];
        ssize_t ret = ::readlink(path.c_str(), linkPath, PATH_MAX);
        if (ret > 0)
        {
            linkPath[ret] = 0;
            linkPath[PATH_MAX] = 0;
            if (linkPath[0] == '/')
            {
                return linkPath;
            }
            else
            {
                return Common::FileSystem::join(dirName(path), linkPath);
            }
        }
        return std::optional<Path>{};
    }

    off_t FileSystemImpl::fileSize(const Path& path) const
    {
        struct stat statbuf; // NOLINT
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        { // if it does not exist
            return -1;
        }
        return statbuf.st_size;
    }

    std::time_t FileSystemImpl::lastModifiedTime(const Path& path) const
    {
        struct stat statbuf; // NOLINT
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        { // if it does not exist
            return -1;
        }
        return statbuf.st_mtim.tv_sec;
    }

    bool FileSystemImpl::compareFileDescriptors(int fd1, int fd2) const
    {
        struct stat statbuf1; // NOLINT
        int ret = fstat(fd1, &statbuf1);
        if (ret != 0)
        { // if it does not exist
            return false;
        }
        struct stat statbuf2; // NOLINT
        ret = fstat(fd2, &statbuf2);
        if (ret != 0)
        { // if it does not exist
            return false;
        }
        if (statbuf1.st_dev == statbuf2.st_dev && statbuf1.st_ino == statbuf2.st_ino)
        {
            return true;
        }
        return false;
    }

    int FileSystemImpl::getFileInfoDescriptor(const Path& path) const
    {
        return open(path.c_str(), O_PATH);
    }

    int FileSystemImpl::getFileInfoDescriptorFromDirectoryFD(int fd, const Path& path) const
    {
        return openat(fd, path.c_str(), O_PATH);
    }

    void FileSystemImpl::unlinkFileUsingDirectoryFD(int fd, const Path& path) const
    {
        if (unlinkat(fd, path.c_str(), 0) != 0)
        {
            int error = errno;

            std::string error_cause = StrError(error);
            throw Common::FileSystem::IFileSystemException(
                "Failed to delete file using directory FD: (relative path)" + path + ". Cause: " + error_cause);
        }
    }

    void FileSystemImpl::unlinkDirUsingDirectoryFD(int fd, const Path& path) const
    {
        if (unlinkat(fd, path.c_str(), AT_REMOVEDIR) != 0)
        {
            int error = errno;

            std::string error_cause = StrError(error);
            throw Common::FileSystem::IFileSystemException(
                "Failed to delete directory entry using directory FD: (relative path) " + path +
                ". Cause: " + error_cause);
        }
    }
    void FileSystemImpl::removeFilesInDirectory(const Path& path) const
    {
        if (!FileSystemImpl::isDirectory(path))
        {
            return;
        }
        std::vector<Path> files = listFiles(path);
        for (auto& file : files)
        {
            removeFile(file);
        }
    }

    bool FileSystemImpl::waitForFile(const Path& path, unsigned int timeout) const
    {
        bool fileExists = false;
        unsigned int waited = 0;
        unsigned int waitPeriod = 1000; // 1ms for use with usleep
        unsigned int target = timeout * 1000;
        while (!(fileExists = exists(path)) && waited < target)
        {
            usleep(waitPeriod);
            waited += waitPeriod;
        }
        return fileExists;
    }
    std::optional<std::string> FileSystemImpl::readProcFile(int pid, const std::string& filename) const
    {
        std::array<char, 4096> buffer{};
        std::string path = Common::FileSystem::join("/proc", std::to_string(pid), filename);
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0)
        {
            return std::optional<std::string>{};
        }
        int nbytes = ::read(fd, buffer.data(), buffer.size());
        ::close(fd);

        if (nbytes == -1)
        {
            return std::optional<std::string>{};
        }
        assert(nbytes <= static_cast<int>(buffer.size()));
        return std::string{ buffer.begin(), buffer.begin() + nbytes };
    }

    std::unique_ptr<Common::FileSystem::IFileSystem>& fileSystemStaticPointer()
    {
        static std::unique_ptr<Common::FileSystem::IFileSystem> instance =
            std::unique_ptr<Common::FileSystem::IFileSystem>(new Common::FileSystem::FileSystemImpl());
        return instance;
    }

    void FileSystemImpl::recursivelyDeleteContentsOfDirectory(const Path& path) const
    {
        std::vector<std::string> filesAndDirectories;
        try
        {
            filesAndDirectories = listFilesAndDirectories(path);
            for (auto& pathToRemove : filesAndDirectories)
            {
                removeFileOrDirectory(pathToRemove);
            }
        }
        catch (IFileSystemException& exception)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to remove all contents of :'" << path << "'"
                     << " Reason: " << exception.what() << std::endl;
            throw IFileSystemException(errorMsg.str());
        }
    }

    std::string FileSystemImpl::calculateDigest(SslImpl::Digest digestName, const Path& path) const
    {
        std::ifstream inFileStream{ path, std::ios::binary };

        if (!inFileStream.is_open() || !inFileStream.good())
        {
            throw IFileSystemException("'" + path + "' does not exist or can't be opened for reading");
        }

        return Common::SslImpl::calculateDigest(digestName, inFileStream);
    }

    std::string FileSystemImpl::calculateDigest(SslImpl::Digest digestName, int fd) const
    {
        // Need to duplicate the file descriptor because the compatibility layer will close it upon destruction
        int dupFd = dup(fd);

        // Set the file offset of the file descriptor to the start, so we calculate the digest of the whole file.
        // Both file descriptors point to the same file description, so changing the file offset for one will change
        // it for the other.
        lseek(dupFd, 0, SEEK_SET);

        // Non-standard GNU compatibility layer between POSIX file descriptors and std::basic_streambuf
        // This also ensures that dupFd gets closed.
        __gnu_cxx::stdio_filebuf<char> fileBuf{ dupFd, std::ios_base::in | std::ios_base::binary };

        if (!fileBuf.is_open())
        {
            throw IFileSystemException("File descriptor '" + std::to_string(fd) + "' can't be opened for reading");
        }

        std::istream inStream{ &fileBuf };

        return Common::SslImpl::calculateDigest(digestName, inStream);
    }

    std::filesystem::space_info FileSystemImpl::getDiskSpaceInfo(const Path& path) const
    {
        return std::filesystem::space(path);
    }

    std::filesystem::space_info FileSystemImpl::getDiskSpaceInfo(const Path& path, std::error_code& ec) const
    {
        return std::filesystem::space(path, ec);
    }

    std::string FileSystemImpl::getSystemCommandExecutablePath(const std::string& executableName) const
    {
        std::vector<std::string> folderLocations = { "/usr/bin", "/bin", "/usr/local/bin", "/sbin", "/usr/sbin" };
        for (const auto& folder : folderLocations)
        {
            Path path = join(folder, executableName);
            if (isExecutable(path))
            {
                return path;
            }
        }
        throw IFileSystemException("Executable " + executableName + " is not installed.");
    }
} // namespace Common::FileSystem

Common::FileSystem::IFileSystem* Common::FileSystem::fileSystem()
{
    return Common::FileSystem::fileSystemStaticPointer().get();
}
