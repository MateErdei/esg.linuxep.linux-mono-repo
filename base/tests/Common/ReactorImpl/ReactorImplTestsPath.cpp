/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ReactorImplTestsPath.h"

#include "Common/FileSystem/IFileSystem.h"
namespace
{
    static std::string programPath;
}

void setProgramPath(std::string programPath_)
{
    programPath = programPath_;
}

std::string ReactorImplTestsPath()
{
#ifdef SPL_BAZEL
    return "tests/Common/ReactorImpl";
#else
    auto fileSystem = Common::FileSystem::fileSystem();
    std::string fullPath;
    if (programPath.front() == '/')
    {
        fullPath = programPath;
    }
    else
    {
        fullPath = Common::FileSystem::join(fileSystem->currentWorkingDirectory(), programPath);
    }
    return Common::FileSystem::dirName(fullPath);
#endif
}
