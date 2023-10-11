/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapper.h"
#include "common/ThreatDetector/SusiSettings.h"

namespace threat_scanner
{
    class ISusiWrapperFactory
    {
    public:
        virtual ISusiWrapperSharedPtr createSusiWrapper(const std::string& scannerConfig) = 0;
        virtual ~ISusiWrapperFactory() = default;

        virtual bool update() = 0;
        virtual bool reload() = 0;
        virtual void shutdown() = 0;
        virtual bool susiIsInitialized() = 0;
        virtual bool updateSusiConfig() = 0;
        virtual bool isMachineLearningEnabled() = 0;
    };

    using ISusiWrapperFactorySharedPtr = std::shared_ptr<ISusiWrapperFactory>;
}
