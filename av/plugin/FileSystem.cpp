/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystem.h"

#include <fstream>
#include <sstream>

#include <cassert>

#include <dirent.h>
#include <sys/stat.h>

namespace
{


    void moveFile(const Path &sourcePath, const Path &destPath)
    {
        if(::rename(sourcePath.c_str(), destPath.c_str()) != 0)
        {
            auto err = errno;
            std::stringstream errorStream;
            errorStream << "Could not move " << sourcePath << " to " << destPath;
            throw std::system_error(err, std::generic_category(), errorStream.str());
        }
    }
}


Path FileSystem::join(const Path& path1, const Path & path2) const
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

void FileSystem::makedirs(const Path &path) const
{
    if (path == "/")
    {
        return;
    }
    if (isDirectory(path))
    {
        return;
    }
    Path p2 = dirPath(path);
    if (path != p2)
    {
        makedirs(p2);
        int ret = ::mkdir(path.c_str(),0700);
        if (ret < 0 && errno != EEXIST)
        {
            throw std::system_error(errno, std::generic_category(), path);
        }
    }
}

std::string FileSystem::readFile(const Path &path) const {
    if(isDirectory(path))
    {
        throw std::system_error( EISDIR, std::generic_category(), path );
    }

    std::ifstream inFileStream(path);

    if (!inFileStream.good())
    {
        throw std::system_error(errno, std::generic_category(), path);
    }

    std::string content((std::istreambuf_iterator<char>(inFileStream)),
                        std::istreambuf_iterator<char>());
    return content;
}

std::vector<Path> FileSystem::listFileSystemEntries(const Path &directoryPath) const {
    static std::string dot{"."};
    static std::string dotdot{".."};
    std::vector<Path> files;
    DIR * directoryPtr = nullptr;

    struct dirent dirEntity; // NOLINT
    struct dirent *outDirEntity = nullptr;

    directoryPtr = opendir(directoryPath.c_str());

    if(!directoryPtr)
    {
        throw std::system_error(errno, std::generic_category(), directoryPath);
    }

    while (true)
    {
        int errorcode = readdir_r(directoryPtr, &dirEntity, &outDirEntity);

        if(errorcode !=0 || !outDirEntity)
        {
            break;
        }

        if ( (DT_REG|DT_DIR) & outDirEntity->d_type &&
             outDirEntity->d_name != dot &&
             outDirEntity->d_name != dotdot )
        {
            std::string fullPath = join(directoryPath, outDirEntity->d_name);
            files.push_back(fullPath);
        }

    }

    (void)closedir(directoryPtr);

    return files;
}

bool FileSystem::isDirectory(const Path &path) const
{
    struct stat statbuf; // NOLINT
    int ret = stat(path.c_str(), &statbuf);
    if ( ret != 0)
    {   // if it does not exists, it is not a directory
        return false;
    }
    return S_ISDIR(statbuf.st_mode); // NOLINT

}

Path FileSystem::baseName(const Path & filePath) const {
    size_t pos = filePath.rfind('/');

    if (pos == Path::npos)
    {
        return filePath;
    }
    return filePath.substr(pos + 1);
}

Path FileSystem::dirPath(const Path & filePath) const {
    std::string tempPath;
    if(filePath.back() == '/')
    {
        tempPath = std::string(filePath.begin(), filePath.end() -1);
    }
    else
    {
        tempPath = filePath;
    }

    size_t pos = tempPath.rfind('/');

    if (pos == Path::npos)
    {
        return ""; // no parent directory
    }

    size_t endPos = filePath.length() - pos;

    return Path(filePath.begin(), (filePath.end() - endPos));
}

bool FileSystem::isRegularFile(const Path &path) const {
    struct stat statbuf; // NOLINT
    int ret = stat(path.c_str(), &statbuf);
    if ( ret != 0)
    {   // if it does not exists, it is not a directory
        return false;
    }
    return S_ISREG(statbuf.st_mode); // NOLINT
}

