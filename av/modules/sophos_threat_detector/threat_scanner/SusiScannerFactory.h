/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScannerFactory.h"

#include "SusiWrapperFactory.h"

namespace threat_scanner
{
    class SusiScannerFactory : public IThreatScannerFactory
    {
    public:
        SusiScannerFactory();
        IThreatScannerPtr createScanner(bool scanArchives) override;
    private:
        SusiWrapperFactorySharedPtr m_wrapperFactory;
    };
}