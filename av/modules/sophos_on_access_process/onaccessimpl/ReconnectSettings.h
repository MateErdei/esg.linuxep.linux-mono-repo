// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

/* Total number of connection attempts to try - scaled to be 4 files + starting on 5th,
 * to cover cases where a particular file is crashing the scanner */
#define TOTAL_MAX_RECONNECTS 250

/* Number of attempts for initial connection to socket, before scanning any file */
#define MAX_CONN_RETRIES 20

#ifdef USING_LIBFUZZER
#define MAX_SCAN_RETRIES 1
#else
/* Number of attempt to scan a particular file, reconnecting if required */
#define MAX_SCAN_RETRIES 60
#endif

/*
 * Timing out OA scan will take (MAX_CONN_RETRIES + TOTAL_MAX_RECONNECTS) * sleepTime(1second)
 */
