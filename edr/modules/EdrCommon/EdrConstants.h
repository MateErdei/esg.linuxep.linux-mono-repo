// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

namespace EdrCommon
{
    // XDR consts
    static const int DEFAULT_MAX_BATCH_SIZE_BYTES = 2000000; // 2MB
    static const int DEFAULT_MAX_BATCH_TIME_SECONDS = 15;
    static const int DEFAULT_XDR_DATA_LIMIT_BYTES = 250000000; // 250MB
    static const int DEFAULT_XDR_PERIOD_SECONDS = 86400; // 1 day
    static const int MAX_PLUGIN_FD_COUNT = 500;
    // If plugin memory exceeds this limit then restart the entire plugin (100 MB)
    static const int MAX_PLUGIN_MEM_BYTES = 100000000;
    static constexpr const char * TELEMETRY_CALLBACK_COOKIE = "EDR plugin";
    // Return codes
    static const int E_STD_EXCEPTION_AT_TOP_LEVEL = 40;
    static const int E_NON_EXCEPTION_AT_TOP_LEVEL = 41;
    static const int E_MEMORY_LIMIT_REACHED = 42;
    static const int E_FD_LIMIT_REACHED = 43;
    static const int RESTART_EXIT_CODE = 77;
}
