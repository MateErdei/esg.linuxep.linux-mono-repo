// Copyright 2022, Sophos Limited.  All rights reserved.

#include <stddef.h>

namespace sophos_on_access_process::OnAccessConfig
{
    const unsigned long int processFdLimit = 1048576;

    const size_t maxAllowedQueueSize = 1048000;
    const size_t minAllowedQueueSize = 1000;
    const int maxScanningThreads = 100;
    const int minScanningThreads = 1;

    const size_t defaultMaxScanQueueSize = 4000;
    const int defaultScanningThreads = 10;
    const bool defaultDumpPerfData = false;
}