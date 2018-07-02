//
// Created by pair on 29/06/18.
//

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
    auto fileSystem = Common::FileSystem::createFileSystem();
    std::string fullPath;
    if ( programPath.front() == '/')
    {
        fullPath = programPath;
    }
    else
    {
        fullPath = fileSystem->join(fileSystem->currentWorkingDirectory(), programPath);
    }
    return fileSystem->dirName(fullPath);
}
