// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once
#include "IQuarantineManager.h"

#include "common/AbstractThreadPluginInterface.h"

using namespace std::chrono_literals;
namespace safestore
{
    class StateMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit StateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager);
        void run() override;

    private:
        std::shared_ptr<IQuarantineManager> m_quarantineManager;
        const std::chrono::seconds m_maxReinitialiseBackoff = 86400s;
    };
} // namespace safestore