/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
namespace common
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN = 0,
        E_GENERIC_FAILURE = 1,
        E_VIRUS_FOUND = 69,
        E_SCAN_ABORTED = 70,
        E_SCAN_ABORTED_WITH_THREATS = 71
    };
}
