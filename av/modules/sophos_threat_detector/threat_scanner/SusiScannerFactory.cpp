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
    IThreatReporterSharedPtr reporter = std::make_shared<ThreatReporter>();
    return std::make_unique<SusiScanner>(m_wrapperFactory, scanArchives, std::move(reporter));
}

SusiScannerFactory::SusiScannerFactory(ISusiWrapperFactorySharedPtr wrapperFactory)
    : m_wrapperFactory(std::move(wrapperFactory))
{
}

SusiScannerFactory::SusiScannerFactory()
    : SusiScannerFactory(std::make_shared<SusiWrapperFactory>())
{
}

bool SusiScannerFactory::update()
{
    return m_wrapperFactory->update();
}