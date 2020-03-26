/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiScanner.h"
#include "ISusiScannerFactory.h"

namespace susi_scanner
{
    class SusiScannerFactory : public ISusiScannerFactory
    {
    public:
        ISusiScannerPtr createScanner() override;
    };
}
