// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once
#include "safestore/QuarantineManager/IQuarantineManager.h"

#include "common/AbstractThreadPluginInterface.h"

#include <atomic>

using namespace std::chrono_literals;
namespace safestore::QuarantineManager
{
    class StateMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit StateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager);
        void run() override;
        ~StateMonitor() override;
        void tryStop() final;

    private:
        std::shared_ptr<IQuarantineManager> m_quarantineManager;
        const std::chrono::seconds m_maxReinitialiseBackoff = 86400s;
        std::chrono::duration<double> m_lastCheck = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds m_reinitialiseBackoff = 60s;
    protected:
        void innerRun();
        std::mutex m_QMCheckLock;
        std::condition_variable m_checkWakeUp;
        std::atomic<bool> m_stopRequested = false;
    };

} // namespace safestore::QuarantineManager