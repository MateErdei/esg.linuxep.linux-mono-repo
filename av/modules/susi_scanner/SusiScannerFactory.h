/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <bits/unique_ptr.h>
#include "SusiScanner.h"

namespace susi_scanner
{
    class IScannerFactory
    {
        public:
            virtual std::unique_ptr<susi_scanner::SusiScanner> createScanner();
    };

    class SusiScannerFactory : public IScannerFactory
    {
        std::unique_ptr<susi_scanner::SusiScanner> createScanner() override;
    };

    class MockScannerFactory : public IScannerFactory
    {
        virtual std::unique_ptr<susi_scanner::SusiScanner> createScanner();
    };
}
