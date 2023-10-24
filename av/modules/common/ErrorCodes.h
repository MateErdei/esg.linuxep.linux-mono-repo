// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once

#include "ErrorCodesC.h"

namespace common
{
    enum E_ERROR_CODES: int
    {
        E_CLEAN_SUCCESS = 0,
        E_FAILURE = 1, // EXIT_FAILURE
        E_BAD_OPTION = 3,
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
        E_SOPHOS_INSTALL_NO_SET_C =  E_SOPHOS_INSTALL_NO_SET, //68
        E_QUICK_RESTART_SUCCESS = 77,
        E_NON_EXCEPTION_AT_TOP_LEVEL = 80,
        E_STD_EXCEPTION_AT_TOP_LEVEL = 81,
        E_BAD_ALLOC = 82,
        E_RUNTIME_ERROR = 83,
        E_IEXCEPTION_AT_TOP_LEVEL = 84,
        E_INNER_MAIN_EXCEPTION = 85,
        E_THREAT_DETECTOR_EXCEPTION = 86,
        E_THREAT_SCANNER_EXCEPTION = 87,
        E_UNIX_SOCKET_EXCEPTION = 88,
        E_AV_EXCEPTION = 89
    };
}
