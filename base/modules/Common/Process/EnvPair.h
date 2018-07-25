/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef COMMON_PROCESS_ENVPAIR_H
#define COMMON_PROCESS_ENVPAIR_H

#include <string>
#include <vector>

namespace Common
{
    namespace Process
    {

        using EnvironmentPair = std::pair<std::string,std::string>;
        using EnvPairVector = std::vector<EnvironmentPair>;
    }
}

#endif //COMMON_PROCESS_ENVPAIR_H
