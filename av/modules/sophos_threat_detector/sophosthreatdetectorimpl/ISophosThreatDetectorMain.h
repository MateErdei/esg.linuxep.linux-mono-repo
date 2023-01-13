// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace sspl::sophosthreatdetectorimpl
{
    class ISophosThreatDetectorMain
    {
    public:
        virtual ~ISophosThreatDetectorMain() = default;

        int sophos_threat_detector_main();

        virtual void reloadSUSIGlobalConfiguration() = 0;
        virtual void shutdownThreatDetector() = 0;
    };

    using ISophosThreatDetectorMainPtr = std::shared_ptr<ISophosThreatDetectorMain>;
}

