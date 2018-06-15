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

#define LOGSUPPORT(x) std::cout << x << "\n";
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
                return path1 + path2.substr(1);
            }
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

        bool FileSystemImpl::exists(const Path &path) const
        {
            struct stat statbuf;
            int ret = stat(path.c_str(), &statbuf);
            return ret == 0;
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