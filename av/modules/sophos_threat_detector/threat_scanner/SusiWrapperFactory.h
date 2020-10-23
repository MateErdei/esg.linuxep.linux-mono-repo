/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapperFactory.h"
#include "SusiGlobalHandler.h"

namespace threat_scanner
{
    class SusiWrapperFactory : public ISusiWrapperFactory
    {
    public:
        SusiWrapperFactory();
        std::shared_ptr<ISusiWrapper> createSusiWrapper(
            const std::string& scannerConfig) override;

        bool update();

    private:
        SusiGlobalHandlerSharePtr m_globalHandler;
    };

    using SusiWrapperFactorySharedPtr = std::shared_ptr<SusiWrapperFactory>;
}
