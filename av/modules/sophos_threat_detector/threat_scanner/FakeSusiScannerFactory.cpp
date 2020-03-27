/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeSusiScannerFactory.h"

#include "FakeSusiScanner.h"

using namespace threat_scanner;

IThreatScannerPtr FakeSusiScannerFactory::createScanner()
{
    return std::make_unique<FakeSusiScanner>();
}
