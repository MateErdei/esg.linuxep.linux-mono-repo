/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScanner.h"
#include <bits/unique_ptr.h>

namespace susi_scanner
{
    class ISusiScannerFactory
    {
    public:
        virtual std::unique_ptr<susi_scanner::SusiScanner> createScanner() = 0;
    };
}
