/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScannerFactory.h"

#include "SusiScanner.h"
#include "SusiWrapperFactory.h"

using namespace threat_scanner;

IThreatScannerPtr SusiScannerFactory::createScanner(bool scanArchives, bool scanImages)
{
    return std::make_unique<SusiScanner>(m_wrapperFactory, scanArchives, scanImages, m_reporter, m_shutdownTimer);
}

SusiScannerFactory::SusiScannerFactory(
    ISusiWrapperFactorySharedPtr wrapperFactory,
    IThreatReporterSharedPtr reporter,
    IScanNotificationSharedPtr shutdownTimer)
    : m_wrapperFactory(std::move(wrapperFactory))
    , m_reporter(std::move(reporter))
    , m_shutdownTimer(shutdownTimer)
{
}

SusiScannerFactory::SusiScannerFactory(IThreatReporterSharedPtr reporter, IScanNotificationSharedPtr shutdownTimer)
    : SusiScannerFactory(std::make_shared<SusiWrapperFactory>(), std::move(reporter), std::move(shutdownTimer))
{
}

bool SusiScannerFactory::update()
{
    return m_wrapperFactory->update();
}

bool SusiScannerFactory::susiIsInitialized()
{
    return m_wrapperFactory->susiIsInitialized();
}
