// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/AbstractThreadPluginInterface.h"
#include "common/ErrorCodes.h"

#include <atomic>


namespace sspl::sophosthreatdetectorimpl
{
    class ProcessForceExitTimer : public common::AbstractThreadPluginInterface
    {
    public:
        explicit ProcessForceExitTimer(std::chrono::seconds timeout);
        ~ProcessForceExitTimer() override;

        /**
         * tryStop() uses local m_stopRequested to interrupt wait_for() in the run method.
         * This differs from normal common::AbstractThreadPluginInterface implementation as we can't use its notifyPipe to
         * request stops, as they would be consumed in the by the wait_for() check and we wouldn't be able to logically
         * check for a stop request.
         */
        void tryStop() final;
        void run() override;

        void setExitCode(int exitCode);

    private:
        std::chrono::seconds m_timeout;
        std::atomic<bool> m_stopRequested = false;
        int m_exitCode = common::E_QUICK_RESTART_SUCCESS;

        std::mutex m_mutex;
        std::condition_variable m_cond;
    };
}