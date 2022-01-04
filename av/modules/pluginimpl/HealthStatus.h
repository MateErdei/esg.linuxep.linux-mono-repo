/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Plugin
{
    enum E_HEALTH_STATUS : long
    {
        E_HEALTH_STATUS_GOOD = 0,
        E_HEALTH_STATUS_BAD = 1,
        E_THREAT_HEALTH_STATUS_GOOD = 1,
        E_THREAT_HEALTH_STATUS_SUSPICIOUS = 2,
    };
}

