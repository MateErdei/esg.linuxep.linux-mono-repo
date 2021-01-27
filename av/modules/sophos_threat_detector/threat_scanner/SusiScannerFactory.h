/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScannerFactory.h"

#include "IThreatReporter.h"
#include "SusiWrapperFactory.h"

namespace threat_scanner
{
    class SusiScannerFactory : public IThreatScannerFactory
    {
    public:
        explicit SusiScannerFactory(IThreatReporterSharedPtr reporter);
        SusiScannerFactory(ISusiWrapperFactorySharedPtr wrapperFactory, IThreatReporterSharedPtr reporter);
        IThreatScannerPtr createScanner(bool scanArchives) override;

        bool update() override;
    private:
        ISusiWrapperFactorySharedPtr m_wrapperFactory;
        IThreatReporterSharedPtr m_reporter;
    };
}