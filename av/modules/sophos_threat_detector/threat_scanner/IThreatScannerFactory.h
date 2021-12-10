/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"

namespace threat_scanner
{
    class IThreatScannerFactory
    {
    public:
        virtual IThreatScannerPtr createScanner(bool scanArchives, bool scanImages) = 0;
        virtual ~IThreatScannerFactory() = default;

        virtual bool update() = 0;
        virtual bool susiIsInitialized() = 0;
    };
    using IThreatScannerFactorySharedPtr = std::shared_ptr<IThreatScannerFactory>;
}
