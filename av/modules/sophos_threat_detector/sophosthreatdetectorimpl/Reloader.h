// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "SafeStoreRescanWorker.h"

#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"

#include "common/signals/IReloadable.h"

#include <stdexcept>
#include <utility>

namespace sspl::sophosthreatdetectorimpl
{
    class Reloader : public common::signals::IReloadable
    {
    public:
        Reloader() = default;

        explicit Reloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,  std::shared_ptr<SafeStoreRescanWorker> safeStoreRescanTrigger)
            : m_scannerFactory(std::move(scannerFactory)), m_safeStoreRescanTrigger(std::move(safeStoreRescanTrigger))
        {}

        void update() override;
        void reload() override;

        void reset(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        {
            m_scannerFactory = std::move(scannerFactory);
        }
    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        std::shared_ptr<SafeStoreRescanWorker> m_safeStoreRescanTrigger;

    };
}
