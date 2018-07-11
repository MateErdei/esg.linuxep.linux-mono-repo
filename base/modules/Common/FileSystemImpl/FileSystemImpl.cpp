/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystemImpl.h"

#include "IFileSystemException.h"

#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <vector>

#define LOGSUPPORT(x) std::cout << x << "\n";


namespace Common
{
    namespace FileSystem
    {

        Path FileSystemImpl::join(const Path& path1, const Path & path2) const
        {
            std::string subPath2;

            if(path2.find("./") == 0 )
            {
                subPath2 = path2.substr(2);
            }
            else
            {
                subPath2 = path2;
            }

            if(path1.back() != '/' && subPath2.front() != '/' )
            {
                return path1 + '/' + subPath2;
            }

            if( (path1.back() != '/' && subPath2.front() == '/' ) || (path1.back() == '/' && subPath2.front() != '/')  )
            {
                return path1 + subPath2;
            }

            if(path1.back() == '/' && subPath2.front() == '/' )
            {
                return path1 + subPath2.substr(1);
            }

            return "";
        }

        std::string FileSystemImpl::basename(const Path & path ) const
        {
            size_t pos = path.rfind('/');

            if (pos == Path::npos)
            {
                return path;
            }
            return path.substr(pos + 1);
        }

        std::string FileSystemImpl::dirName(const Path & path) const
        {
            std::string tempPath;
            if(path.back() == '/')
            {
                tempPath = std::string(path.begin(), path.end() -1);
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

            int endPos = path.length() - pos;

            return Path(path.begin(), (path.end() - endPos));
        }

        bool FileSystemImpl::exists(const Path &path) const
        {
            struct stat statbuf;
            int ret = stat(path.c_str(), &statbuf);
            return ret == 0;
        }

        bool FileSystemImpl::isFile(const Path & path) const
        {
            struct stat statbuf;
            int ret = stat(path.c_str(), &statbuf);
            if ( ret != 0)
            {   // if it does not exists, it is not a file
                return false;
            }
            return S_ISREG(statbuf.st_mode);
        }


        bool FileSystemImpl::isDirectory(const Path & path) const
        {
            struct stat statbuf;
            int ret = stat(path.c_str(), &statbuf);
            if ( ret != 0)
            {   // if it does not exists, it is not a directory
                return false;
            }
            return S_ISDIR(statbuf.st_mode);
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

            return currentWorkingDirectory;
        }

        void FileSystemImpl::moveFile(const Path &sourcePath, const Path &destPath) const
        {
            if(::rename(sourcePath.c_str(), destPath.c_str()) != 0)
            {
                int err = errno;
                std::string errdesc = ::strerror(err);

                throw IFileSystemException(errdesc);
            }
        }

        std::string FileSystemImpl::readFile(const Path &path) const
        {
            if(isDirectory(path))
            {
                throw IFileSystemException("Error, Failed to read file: '" + path + "', is a directory");
            }

            std::ifstream inFileStream(path);

            if (!inFileStream.good())
            {
                throw IFileSystemException("Error, Failed to read file: '" + path + "', file does not exist");
            }

            try
            {
                std::string content((std::istreambuf_iterator<char>(inFileStream)),
                        std::istreambuf_iterator<char>());
                return content;
            }
            catch( std::system_error & ex)
            {
                LOGSUPPORT(ex.what());
                throw IFileSystemException(std::string("Error, Failed to read from file '") + path + "'");
            }

        }

        void FileSystemImpl::writeFile(const Path &path, const std::string &content) const
        {
            std::ofstream outFileStream(path.c_str(), std::ios::out);

            if(!outFileStream.good())
            {
                int err = errno;
                std::string errdesc = ::strerror(err);

                throw IFileSystemException("Error, Failed to create file: '" + path + "', " + errdesc);
            }

            outFileStream << content;

            outFileStream.close();

        }

        void FileSystemImpl::writeFileAtomically(const Path &path, const std::string &content, const Path &tempDir) const
        {
            if(!isDirectory(tempDir))
            {
                throw IFileSystemException("Temp directory provided is not a directory");
            }

            // create temp file name.
            std::string tempFilePath =join( tempDir, basename(path) + "_tmp");

            try
            {
                writeFile(tempFilePath, content);

                moveFile(tempFilePath, path);

            }
            catch(IFileSystemException &)
            {
                remove(tempFilePath.c_str());
                throw;
            }
        }

        bool FileSystemImpl::isExecutable(const Path &path) const
        {
            struct stat statbuf;
            int ret = stat(path.c_str(), &statbuf);
            if ( ret != 0)
            {   // if it does not exists, it is not executable
                return false;
            }
            return S_IXUSR & statbuf.st_mode;
        }

        std::vector<Path> FileSystemImpl::listFiles(const Path &directoryPath) const
        {
            std::vector<Path> files;
            DIR * directoryPtr;

            struct dirent dirEntity;
            struct dirent *outDirEntity;

            directoryPtr = opendir(directoryPath.c_str());

            if(!directoryPtr)
            {
                int error = errno;
                std::string reason = strerror(error);
                throw IFileSystemException("Failed to read directory: '" + directoryPath + "', error:  " + reason);
            }


            while (true)
            {
                int errorcode = readdir_r(directoryPtr, &dirEntity, &outDirEntity);

                if(errorcode !=0 || !outDirEntity)
                {
                    break;
                }

                if ( DT_REG & outDirEntity->d_type )
                {
                    std::string fullPath = join(directoryPath, outDirEntity->d_name);
                    files.push_back(fullPath);
                }

            }

            (void)closedir(directoryPtr);

            return files;
        }
    }
}

namespace
{
    Common::FileSystem::IFileSystemPtr& fileSystemStaticPointer()
    {
        static Common::FileSystem::IFileSystemPtr instance = Common::FileSystem::IFileSystemPtr(new Common::FileSystem::FileSystemImpl());
        return instance;
    }
}

Common::FileSystem::IFileSystem * Common::FileSystem::fileSystem()
{
    return fileSystemStaticPointer().get();
}

void Common::FileSystem::replaceFileSystem(Common::FileSystem::IFileSystemPtr pointerToReplace)
{
    fileSystemStaticPointer().reset(pointerToReplace.release());
}

void Common::FileSystem::restoreFileSystem()
{
    fileSystemStaticPointer().reset( new Common::FileSystem::FileSystemImpl());
}


