// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IThreatScanner.h"

namespace threat_scanner
{
    class IThreatScannerFactory
    {
    public:
        virtual IThreatScannerPtr createScanner(bool scanArchives, bool scanImages, bool detectPUAs) = 0;
        virtual ~IThreatScannerFactory() = default;

        virtual bool update() = 0;
        virtual bool reload() = 0;
        virtual void shutdown() = 0;
        virtual bool susiIsInitialized() = 0;
        virtual bool updateSusiConfig() = 0;
        virtual bool detectPUAsEnabled() = 0;
        virtual void loadSusiSettingsIfRequired() = 0;
    };
    using IThreatScannerFactorySharedPtr = std::shared_ptr<IThreatScannerFactory>;
}
