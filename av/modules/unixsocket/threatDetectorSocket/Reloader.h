/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReloadable.h"

#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

namespace unixsocket
{
    class Reloader : public unixsocket::IReloadable
    {
    public:
        explicit Reloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
            : m_scannerFactory(std::move(scannerFactory))
        {}
        void reload() override
        {
            if (!m_scannerFactory->update())
            {
                throw std::runtime_error("Failed to update threat scanner");
            }
        }
    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;

    };
}
