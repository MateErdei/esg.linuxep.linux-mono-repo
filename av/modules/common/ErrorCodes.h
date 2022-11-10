/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ErrorCodesC.h"

namespace common
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN_SUCCESS = 0,
        E_GENERIC_FAILURE = 8,
        E_SIGTERM = 15,
        E_PASSWORD_PROTECTED = 16,
        E_FILE_DISINFECTED = 20,
        E_VIRUS_FOUND = 24,
        E_SCAN_ABORTED_WITH_THREATS = 25,
        E_MEMORY_THREAT = 28,
        E_INTEGRITY_CHECK_FAILURE = 32,
        E_SCAN_ABORTED = 36,
        E_EXECUTION_INTERRUPTED = 40,
        E_EXECUTION_MANUALLY_INTERRUPTED = 41,
        E_CAP_GET_PROC_C    = E_CAP_GET_PROC,   // 60
        E_CAP_SET_FLAG_C    = E_CAP_SET_FLAG,   // 61
        E_CAP_SET_PROC_C    = E_CAP_SET_PROC,   // 62
        E_CAP_FREE_C        = E_CAP_FREE,       // 63
        E_CAP_SET_AMBIENT_C = E_CAP_SET_AMBIENT,// 64
        E_QUICK_RESTART_SUCCESS = 77
    };
}
