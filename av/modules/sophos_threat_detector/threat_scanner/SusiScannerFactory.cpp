/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScannerFactory.h"

#include "SusiScanner.h"
#include "SusiWrapperFactory.h"
#include "ThreatReporter.h"

using namespace threat_scanner;

IThreatScannerPtr SusiScannerFactory::createScanner(bool scanArchives)
{
    return std::make_unique<SusiScanner>(m_wrapperFactory, scanArchives, m_reporter);
}

SusiScannerFactory::SusiScannerFactory(
    ISusiWrapperFactorySharedPtr wrapperFactory
    , IThreatReporterSharedPtr reporter)
    : m_wrapperFactory(std::move(wrapperFactory)), m_reporter(std::move(reporter))
{
}

SusiScannerFactory::SusiScannerFactory(IThreatReporterSharedPtr reporter)
    : SusiScannerFactory(std::make_shared<SusiWrapperFactory>(), std::move(reporter))
{
}

bool SusiScannerFactory::update()
{
    return m_wrapperFactory->update();
}