/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"
#include "ISusiScannerFactory.h"

namespace susi_scanner
{
    class SusiScannerFactory : public ISusiScannerFactory
    {
    public:
        IThreatScannerPtr createScanner() override;
    };
}
