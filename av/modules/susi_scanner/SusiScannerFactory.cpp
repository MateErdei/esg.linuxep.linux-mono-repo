/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScannerFactory.h"

std::unique_ptr<susi_scanner::SusiScanner> susi_scanner::SusiScannerFactory::createScanner()
{
    return std::make_unique<susi_scanner::SusiScanner>();
}
