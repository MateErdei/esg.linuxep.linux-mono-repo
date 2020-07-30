/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScannerFactory.h"

#include "SusiScanner.h"
#include "SusiWrapperFactory.h"

using namespace threat_scanner;

IThreatScannerPtr SusiScannerFactory::createScanner(bool scanArchives)
{
    return std::make_unique<SusiScanner>(m_wrapperFactory, scanArchives);
}

SusiScannerFactory::SusiScannerFactory(ISusiWrapperFactorySharedPtr wrapperFactory)
    : m_wrapperFactory(std::move(wrapperFactory))
{
}

SusiScannerFactory::SusiScannerFactory()
    : SusiScannerFactory(std::make_shared<SusiWrapperFactory>())
{
}

