/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"

namespace threat_scanner
{
    class IThreatScannerFactory
    {
    public:
        virtual IThreatScannerPtr createScanner() = 0;
    };
}
