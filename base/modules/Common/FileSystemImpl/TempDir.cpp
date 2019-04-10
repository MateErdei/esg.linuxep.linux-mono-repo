/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TempDir.h"

#include <Common/FileSystem/IFileSystemException.h>

#include <cstring>
#include <unistd.h>

using namespace Common::FileSystemImpl;

TempDir::TempDir(const std::string& baseDirectory, const std::string& namePrefix)
{
    auto filesystem = Common::FileSystem::fileSystem();
    std::string template_name;
    if (baseDirectory.empty())
    {
        template_name = filesystem->currentWorkingDirectory();
    }
    else
    {
        template_name = baseDirectory;
    }
    template_name = Common::FileSystem::join(template_name, namePrefix);
    template_name += "XXXXXX";
    std::vector<char> aux_array(template_name.begin(), template_name.end());
    aux_array.emplace_back('\0');

    char* ptr2data = ::mkdtemp(aux_array.data());
    if (ptr2data == nullptr)
    {
        int errn = errno;
        std::string error_cause = ::strerror(errn);
        throw Common::FileSystem::IFileSystemException(
                "Failed to create directory: " + template_name + ". Cause: " + error_cause);
    }
    m_tempdir = Path(ptr2data);
}

TempDir::~TempDir()
{
    deleteTempDir();
}

void TempDir::deleteTempDir()
{
    deleteDirectory(m_tempdir);
}

void TempDir::deleteDirectory(const std::string& dir, Common::FileSystem::IFileSystem* filesystem)
{
    if (filesystem == nullptr)
    {
        filesystem = Common::FileSystem::fileSystem();
    }
    for (const auto& path : filesystem->listFilesAndDirectories(dir, true))
    {
        if (filesystem->isSymlink(path) || filesystem->isFile(path))
        {
            filesystem->removeFile(path);
        }
        else if (filesystem->isDirectory(path))
        {
            deleteDirectory(path, filesystem);
        }
    }

    filesystem->removeFile(dir);
}
