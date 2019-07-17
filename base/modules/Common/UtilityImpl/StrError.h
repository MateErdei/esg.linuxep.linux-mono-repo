/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
namespace Common
{
    namespace UtilityImpl
    {
        /** Implements a thread safe retrieval of strerror */
        std::string StrError(int errNumber);
    }
}



