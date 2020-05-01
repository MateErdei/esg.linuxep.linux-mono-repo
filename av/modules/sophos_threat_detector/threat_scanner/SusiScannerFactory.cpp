/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScannerFactory.h"

#include "SusiScanner.h"
#include "SusiWrapperFactory.h"

using namespace threat_scanner;

IThreatScannerPtr SusiScannerFactory::createScanner()
{
    return std::make_unique<SusiScanner>(std::make_shared<SusiWrapperFactory>());
}