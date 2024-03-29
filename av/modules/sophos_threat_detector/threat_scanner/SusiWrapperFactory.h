// Copyright 2020-2022 Sophos Limited. All rights reserved.

#pragma once

#include "ISusiWrapperFactory.h"
#include "SusiGlobalHandler.h"

namespace threat_scanner
{
    class SusiWrapperFactory : public ISusiWrapperFactory
    {
    public:
        SusiWrapperFactory();
        std::shared_ptr<ISusiWrapper> createSusiWrapper(const std::string& scannerConfig) override;

        bool update() override;
        bool susiIsInitialized() override;
        bool reload() override;
        void shutdown() override;
        bool updateSusiConfig() override;
        bool isMachineLearningEnabled() override
        {
            return m_globalHandler->isMachineLearningEnabled();
        }

        ISusiGlobalHandlerSharedPtr accessGlobalHandler() override
        {
            return m_globalHandler;
        }

    private:
        SusiGlobalHandlerSharedPtr m_globalHandler;
    };

    using SusiWrapperFactorySharedPtr = std::shared_ptr<SusiWrapperFactory>;
} // namespace threat_scanner
