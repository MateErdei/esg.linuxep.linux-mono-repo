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
        long timeout() override;

    private:
        long getDefaultTimeout();
        long getCurrentEpoch();

        std::atomic<long> m_scanStartTime;
        fs::path m_configFile;
    };
}
