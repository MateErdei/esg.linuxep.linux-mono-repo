// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IThreatDetectorResources.h"

namespace sspl::sophosthreatdetectorimpl
{
    class ThreatDetectorResources : public IThreatDetectorResources
    {
        public:
            datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper() override;
            common::signals::ISignalHandlerSharedPtr createSignalHandler(bool _restartSyscalls) override;
            common::IPidLockFileSharedPtr createPidLockFile(const std::string& _path) override;
    };
}