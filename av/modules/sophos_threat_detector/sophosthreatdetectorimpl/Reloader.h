/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

#include "common/signals/IReloadable.h"

#include <stdexcept>

namespace sspl::sophosthreatdetectorimpl
{
    class Reloader : public IReloadable
    {
    public:
        Reloader() = default;

        explicit Reloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
            : m_scannerFactory(std::move(scannerFactory))
        {}

        void update() override;
        void reload() override;

        void reset(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        {
            m_scannerFactory = std::move(scannerFactory);
        }
    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;

    };
}
