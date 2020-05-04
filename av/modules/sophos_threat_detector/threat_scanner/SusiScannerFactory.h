/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScannerFactory.h"

namespace threat_scanner
{
    class SusiScannerFactory : public IThreatScannerFactory
    {
    public:
        IThreatScannerPtr createScanner(bool scanArchives) override;
    };
}