/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"
#include "IThreatScannerFactory.h"

namespace susi_scanner
{
    class SusiScannerFactory : public IThreatScannerFactory
    {
    public:
        IThreatScannerPtr createScanner() override;
    };
}
