/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScannerFactory.h"

#include "SusiScanner.h"

using namespace susi_scanner;

ISusiScannerPtr SusiScannerFactory::createScanner()
{
    return std::make_unique<SusiScanner>();
}
