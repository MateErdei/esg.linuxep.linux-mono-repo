// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IThreatScannerFactory.h"

#include "IScanNotification.h"
#include "IThreatReporter.h"
#include "IUpdateCompleteCallback.h"
#include "SusiWrapperFactory.h"

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace threat_scanner
{
    class SusiScannerFactory : public IThreatScannerFactory
    {
    TEST_PUBLIC:
        SusiScannerFactory(ISusiWrapperFactorySharedPtr wrapperFactory,
                           IThreatReporterSharedPtr reporter,
                           IScanNotificationSharedPtr shutdownTimer,
                           IUpdateCompleteCallbackPtr updateCompleteCallback);

    public:
        SusiScannerFactory(IThreatReporterSharedPtr reporter,
                           IScanNotificationSharedPtr shutdownTimer,
                           IUpdateCompleteCallbackPtr updateCompleteCallback);

        IThreatScannerPtr createScanner(bool scanArchives, bool scanImages) override;

        bool update() override;
        ReloadResult reload() override;
        void shutdown() override;
        bool susiIsInitialized() override;

    private:
        ISusiWrapperFactorySharedPtr m_wrapperFactory;
        IThreatReporterSharedPtr m_reporter;
        IScanNotificationSharedPtr m_shutdownTimer;
        IUpdateCompleteCallbackPtr m_updateCompleteCallback;
    };
}