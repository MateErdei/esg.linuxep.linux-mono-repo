/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
namespace Plugin
{
    std::string osqueryBinaryName();
    std::string osqueryPath();
    std::string osqueryFlagsFilePath();
    std::string osqueryConfigFilePath();
    std::string osquerySocket();
    std::string lockFilePath();
    std::string osqueryPidFile();
    std::string osQueryLogPath();
    std::string osQueryDataBasePath();
    std::string getEdrVersionIniFilePath();

}

