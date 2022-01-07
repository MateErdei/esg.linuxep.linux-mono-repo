/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ISysCalls.h"

namespace datatypes
{
    class SysCallsImpl : public ISysCalls
    {
        public:
            /** A wrapper for sysinfo that returns success/failure and system uptime
            *
            * @param none
            * @return 0, uptime on success and -1, 0 on error
            */
            std::pair<const int, const long> getSystemUpTime() override;
    };
}