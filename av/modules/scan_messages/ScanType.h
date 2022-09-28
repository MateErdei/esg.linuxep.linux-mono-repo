//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

namespace scan_messages
{
    enum E_SCAN_TYPE : int
    {
        E_SCAN_TYPE_UNKNOWN = 200,
        E_SCAN_TYPE_ON_ACCESS = 201,
        E_SCAN_TYPE_ON_DEMAND = 203,
        E_SCAN_TYPE_SCHEDULED = 205,
        E_SCAN_TYPE_MEMORY = 206,

        //For products internal use
        E_SCAN_TYPE_ON_ACCESS_OPEN = 207,
        E_SCAN_TYPE_ON_ACCESS_CLOSE = 208
    };
}

