/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/IScanNotification.h"

#include "datatypes/sophos_filesystem.h"
#include <atomic>

namespace fs = sophos_filesystem;

namespace sspl::sophosthreatdetectorimpl
{
    class ShutdownTimer : public threat_scanner::IScanNotification
    {
    public:
        ShutdownTimer(fs::path configFile);
        void reset() override;
        time_t timeout() override;

    private:
        time_t getDefaultTimeout();
        time_t getCurrentEpoch();

        std::atomic<time_t> m_scanStartTime;
        fs::path m_configFile;
    };
}
