/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystemImpl.h"

#include "IFileSystemException.h"

#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <cstring>


namespace Common
{
    namespace FileSystem
    {
        FileSystemImpl::FileSystemImpl()
        {

        }

        FileSystemImpl::~FileSystemImpl()
        {

        }

        Path FileSystemImpl::join(const Path& path1, const Path & path2) const
        {
            if(path1.back() != '/' && path2.front() != '/' )
            {
                return path1 + '/' + path2;
            }

            if( (path1.back() != '/' && path2.front() == '/' )
                || path1.back() == '/' && path2.front() != '/' )
            {
                return path1 + path2;
            }

            if(path1.back() == '/' && path2.front() == '/' )
            {
                return path1 + '/' + std::string( path2.begin()+1, path2.end());
            }
        }

        std::string FileSystemImpl::basename(const Path & path ) const
        {
            size_t pos = path.rfind('/');

            if (pos == Path::npos)
            {
                return path;
            }
            return Path(path.begin() + pos  + 1, path.end());
        }

        bool FileSystemImpl::exists(const Path &path) const
        {
            std::ifstream fileStream(path);
            return fileStream.good();
        }

        bool FileSystemImpl::isDirectory(const Path & path) const
        {
            // if valid directory a pointer to the directory stream will be returned.
            auto * dir = ::opendir(path.c_str());
            if(dir != nullptr)
            {
                closedir(dir);
                return true;
            }

            return false;
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

        void FileSystemImpl::moveFile(const Path &sourcePath, const Path &destPath)
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
            std::ifstream inFileStream(path);

            if (!inFileStream.good())
            {
                throw IFileSystemException("Error, Failed to read file: '" + path + "', file does not exist");
            }

            std::string content((std::istreambuf_iterator<char>(inFileStream)),
                                std::istreambuf_iterator<char>());

            return content;
        }

        void FileSystemImpl::writeFile(const Path &path, const std::string &content)
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

        void FileSystemImpl::writeFileAtomically(const Path &path, const std::string &content, const Path &tempDir)
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
    }
}