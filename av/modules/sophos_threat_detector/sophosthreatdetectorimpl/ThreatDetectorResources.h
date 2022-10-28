// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IThreatDetectorResources.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorResources : public IThreatDetectorResources
    {
        public:
            datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() override;
    };
}