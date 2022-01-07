/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SysCallsImpl.h"

#include <sys/sysinfo.h>

using namespace datatypes;


std::pair<const int, const long> SysCallsImpl::getSystemUpTime()
{
    struct sysinfo systemInfo;
    int res = sysinfo(&systemInfo);
    long upTime = res == -1 ? 0 : systemInfo.uptime;
    return std::pair(res, upTime);
}