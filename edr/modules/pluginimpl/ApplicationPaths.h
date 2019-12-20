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
    std::string pidFile();
//    std::string tempPath();
    std::string getEdrVersionIniFilePath();

}

