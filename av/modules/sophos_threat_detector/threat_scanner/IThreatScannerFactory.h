/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"
#include "SusiReloadResult.h"

namespace threat_scanner
{
    class IThreatScannerFactory
    {
    public:
        virtual IThreatScannerPtr createScanner(bool scanArchives, bool scanImages) = 0;
        virtual ~IThreatScannerFactory() = default;

        virtual bool update() = 0;
        virtual ReloadResult reload() = 0;
        virtual void shutdown() = 0;
        virtual bool susiIsInitialized() = 0;
        virtual bool hasConfigChanged() = 0;
    };
    using IThreatScannerFactorySharedPtr = std::shared_ptr<IThreatScannerFactory>;
}
