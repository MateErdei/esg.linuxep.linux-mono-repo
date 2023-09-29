/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
namespace Plugin
{
    std::string sessionsDirectoryPath();
    std::string liveResponseTempPath();
    std::string getVersionIniFilePath();
    std::string liveResponseExecutable();
} // namespace Plugin