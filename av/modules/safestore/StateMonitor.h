// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once
#include "IQuarantineManager.h"

#include "common/AbstractThreadPluginInterface.h"
namespace safestore
{
    class StateMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit StateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager);
        void run() override;

    private:
        std::shared_ptr<IQuarantineManager> m_quarantineManager;
    };
} // namespace safestore