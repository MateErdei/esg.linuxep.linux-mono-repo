/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanCallbackImpl.h"

#include "Logger.h"

#include "common/StringUtils.h"

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

void ScanCallbackImpl::cleanFile(const path&)
{

}

void ScanCallbackImpl::infectedFile(const path& p, const std::string& threatName, bool isSymlink)
{
    std::string escapedPath(p);
    common::escapeControlCharacters(escapedPath);

    if (isSymlink)
    {
        std::string escapedTargetPath(fs::absolute(fs::read_symlink(p)));
        common::escapeControlCharacters(escapedTargetPath);
        LOGWARN("Detected \"" << escapedPath << "\" (symlinked to " << escapedTargetPath << ") is infected with " << threatName);
    }
    else
    {
        LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName);
    }
    m_returnCode = E_VIRUS_FOUND;
}