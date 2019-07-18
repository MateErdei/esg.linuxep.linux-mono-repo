/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TempDir.h"

#include "FileSystemImpl.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/StrError.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

using namespace Common::FileSystemImpl;
using namespace Common::UtilityImpl;
TempDir::TempDir(const std::string& baseDirectory, const std::string& namePrefix)
{
    std::string template_name;
    if (baseDirectory.empty())
    {
        Common::FileSystem::FileSystemImpl filesystem;
        template_name = filesystem.currentWorkingDirectory();
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
        int error = errno;
        std::string error_cause = StrError(error);
        throw Common::FileSystem::IFileSystemException(
            "Failed to create directory: " + template_name + ". Cause: " + error_cause);
    }
    m_tempdir = Path(ptr2data);
}

TempDir::~TempDir()
{
    try
    {
        deleteTempDir();
    }
    catch (const std::exception& ex)
    {
        // Can't rely on logging to be setup here.
        std::cerr << "Failed to delete temp directory: " << m_tempdir << " with " << ex.what() << std::endl;
    }
}

void TempDir::deleteTempDir()
{
    // Must use our own instance, because the filesystem may go away before TempDir is deleted. e.g. TestLoggerConfig
    Common::FileSystem::FileSystemImpl filesystem;
    filesystem.removeFileOrDirectory(m_tempdir);
}
