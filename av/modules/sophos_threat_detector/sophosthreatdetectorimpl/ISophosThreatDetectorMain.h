// Copyright 2023, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace sspl::sophosthreatdetectorimpl
{
    class ISophosThreatDetectorMain
    {
    public:
        virtual ~ISophosThreatDetectorMain() = default;

        virtual void reloadSUSIGlobalConfiguration() = 0;
        virtual void shutdownThreatDetector() = 0;
    };
}

