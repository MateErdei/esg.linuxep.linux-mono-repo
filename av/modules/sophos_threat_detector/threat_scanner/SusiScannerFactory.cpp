// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "SusiScannerFactory.h"

#include "Logger.h"
#include "ScannerInfo.h"
#include "SusiScanner.h"
#include "SusiWrapperFactory.h"
#include "UnitScanner.h"

#include <utility>

namespace threat_scanner
{
    IThreatScannerPtr SusiScannerFactory::createScanner(bool scanArchives, bool scanImages)
    {
        std::string scannerConfig = "{" + createScannerInfo(scanArchives, scanImages) + "}";
        auto unitScanner = std::make_unique<UnitScanner>(m_wrapperFactory->createSusiWrapper(scannerConfig));
        return std::make_unique<SusiScanner>(std::move(unitScanner), m_reporter, m_shutdownTimer);
    }

    SusiScannerFactory::SusiScannerFactory(
        ISusiWrapperFactorySharedPtr wrapperFactory,
        IThreatReporterSharedPtr reporter,
        IScanNotificationSharedPtr shutdownTimer,
        IUpdateCompleteCallbackPtr updateCompleteCallback)
        : m_wrapperFactory(std::move(wrapperFactory))
        , m_reporter(std::move(reporter))
        , m_shutdownTimer(std::move(shutdownTimer))
        , m_updateCompleteCallback(std::move(updateCompleteCallback))
    {
    }

    SusiScannerFactory::SusiScannerFactory(
        IThreatReporterSharedPtr reporter,
        IScanNotificationSharedPtr shutdownTimer,
        IUpdateCompleteCallbackPtr updateCompleteCallback)
        : SusiScannerFactory(
            std::make_shared<SusiWrapperFactory>(),
            std::move(reporter),
            std::move(shutdownTimer),
            std::move(updateCompleteCallback))
    {
    }

    bool SusiScannerFactory::update()
    {
        bool updated = m_wrapperFactory->update();
        if (updated && m_updateCompleteCallback)
        {
            LOGDEBUG("Notify clients that the update has completed");
            m_updateCompleteCallback->updateComplete();
        }
        else if (!updated)
        {
            LOGWARN("SUSI Update failed");
        }
        else if (!m_updateCompleteCallback)
        {
            LOGDEBUG("No callback to notify update complete");
        }
        return updated;
    }

    bool SusiScannerFactory::reload()
    {
        return m_wrapperFactory->reload();
    }

    void SusiScannerFactory::shutdown()
    {
        m_wrapperFactory->shutdown();
    }

    bool SusiScannerFactory::susiIsInitialized()
    {
        return m_wrapperFactory->susiIsInitialized();
    }
}