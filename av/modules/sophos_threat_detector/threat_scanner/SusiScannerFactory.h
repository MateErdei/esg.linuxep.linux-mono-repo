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
        explicit SusiScannerFactory(ISusiWrapperFactorySharedPtr wrapperFactory);
        IThreatScannerPtr createScanner(bool scanArchives) override;

        bool update() override;
    private:
        ISusiWrapperFactorySharedPtr m_wrapperFactory;
    };
}