// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

namespace
{
    class FakeThreatScannerFactory : public threat_scanner::IThreatScannerFactory
    {
        bool update() override
        {
            return true;
        }

        bool reload() override
        {
            return true;
        }

        void shutdown() override
        {
        }

        bool susiIsInitialized() override
        {
            return true;
        }

        bool updateSusiConfig() override
        {
            return false;
        }

        bool detectPUAsEnabled() override
        {
            return true;
        }

        void loadSusiSettingsIfRequired() override
        {
        }
    };
}