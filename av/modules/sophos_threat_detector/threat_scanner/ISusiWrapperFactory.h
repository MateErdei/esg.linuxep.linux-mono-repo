/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapper.h"

namespace threat_scanner
{
    class ISusiWrapperFactory
    {
    public:
        virtual ISusiWrapperSharedPtr createSusiWrapper(const std::string& scannerConfig) = 0;
        virtual ~ISusiWrapperFactory() = default;

        virtual bool update() = 0;
        virtual bool susiIsInitialized() = 0;
    };

    using ISusiWrapperFactorySharedPtr = std::shared_ptr<ISusiWrapperFactory>;
}
