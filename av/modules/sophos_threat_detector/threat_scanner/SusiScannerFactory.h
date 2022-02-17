/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScannerFactory.h"

#include "IScanNotification.h"
#include "IThreatReporter.h"
#include "SusiWrapperFactory.h"

namespace threat_scanner
{
    class SusiScannerFactory : public IThreatScannerFactory
    {
    public:
        explicit SusiScannerFactory(IThreatReporterSharedPtr reporter, IScanNotificationSharedPtr shutdownTimer);
        SusiScannerFactory(ISusiWrapperFactorySharedPtr wrapperFactory,
                           IThreatReporterSharedPtr reporter,
                           IScanNotificationSharedPtr shutdownTimer);
        IThreatScannerPtr createScanner(bool scanArchives, bool scanImages) override;

        bool update() override;
        bool reload() override;
        bool susiIsInitialized() override;

    private:
        ISusiWrapperFactorySharedPtr m_wrapperFactory;
        IThreatReporterSharedPtr m_reporter;
        IScanNotificationSharedPtr m_shutdownTimer;
    };
}