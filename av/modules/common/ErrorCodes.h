/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
namespace common
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN = 0,
        E_GENERIC_FAILURE = 8,
        E_PASSWORD_PROTECTED = 16,
        E_VIRUS_FOUND = 24,
        E_SCAN_ABORTED_WITH_THREATS = 25,
        E_SCAN_ABORTED = 36,
        E_EXECUTION_INTERRUPTED = 40
    };
}
