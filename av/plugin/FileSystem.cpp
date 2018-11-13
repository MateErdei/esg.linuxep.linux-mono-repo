/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileSystem.h"
#include <Common/FileSystem/IFileSystem.h>

#include <fstream>
#include <sstream>

#include <cassert>

#include <dirent.h>
#include <sys/stat.h>

Path FileSystem::join(const Path& path1, const Path & path2) const
{
    return Common::FileSystem::join(path1, path2);
}

void FileSystem::makedirs(const Path &path) const
{
    return Common::FileSystem::fileSystem()->makedirs(path);
}

std::string FileSystem::readFile(const Path &path) const
{
    return Common::FileSystem::fileSystem()->readFile(path);
}

std::vector<Path> FileSystem::listFileSystemEntries(const Path &directoryPath) const
{
    return Common::FileSystem::fileSystem()->listFilesAndDirectories(directoryPath);
}

bool FileSystem::isDirectory(const Path &path) const
{
    return Common::FileSystem::fileSystem()->isDirectory(path);
}

Path FileSystem::baseName(const Path & filePath) const
{
    return Common::FileSystem::basename(filePath);
}

Path FileSystem::dirPath(const Path & filePath) const
{
    return Common::FileSystem::dirName(filePath);
}

bool FileSystem::isRegularFile(const Path &path) const
{
    return Common::FileSystem::fileSystem()->isFile(path);
}

