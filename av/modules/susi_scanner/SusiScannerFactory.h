/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <bits/unique_ptr.h>
#include "SusiScanner.h"

namespace susi_scanner
{
    class SusiScannerFactory : public ISusiScannerFactory
    {
        public:
            std::unique_ptr<susi_scanner::SusiScanner> createScanner() override;
    };
}
