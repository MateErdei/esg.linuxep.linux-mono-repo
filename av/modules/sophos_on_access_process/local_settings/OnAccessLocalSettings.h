// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "OnAccessProductConfigDefaults.h"

namespace sophos_on_access_process::local_settings
{
    struct OnAccessLocalSettings
    {
        size_t maxScanQueueSize = defaultMaxScanQueueSize;
        int numScanThreads = defaultScanningThreads;
        bool dumpPerfData = defaultDumpPerfData;
        bool cacheAllEvents = defaultCacheAllEvents;
        bool uncacheDetections = defaultUncacheDetections;
        bool highPrioritySoapd = defaultHighPrioritySoapd;

        /**
         * Set threat detector to high priority
         * This is a setting in soapd, as soapd runs as root, so can make TD high priority
         */
        bool highPriorityThreatDetector = defaultHighPriorityThreatDetector;
    };
}
