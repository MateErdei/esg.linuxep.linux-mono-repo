/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

namespace common
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN_SUCCESS = 0,
        E_GENERIC_FAILURE = 1,
//      signals occupy numbers up to 65
        E_SIGTERM = 15,
        E_VIRUS_FOUND = 69,
        E_SCAN_ABORTED = 70,
        E_SCAN_ABORTED_WITH_THREATS = 71
    };
}
