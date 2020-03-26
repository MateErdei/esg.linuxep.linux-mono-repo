/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiScanner.h"

namespace susi_scanner
{
    class ISusiScannerFactory
    {
    public:
        virtual ISusiScannerPtr createScanner() = 0;
    };
}
