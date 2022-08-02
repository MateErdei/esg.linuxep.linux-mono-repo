/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReloadable.h"

#include "common/signals/SignalHandlerBase.h"

#include <Common/Threads/NotifyPipe.h>

namespace sspl::sophosthreatdetectorimpl
{
    class SigUSR1Monitor
    {
    public:
        SigUSR1Monitor() = delete;
        SigUSR1Monitor(const SigUSR1Monitor&) = delete;
        explicit SigUSR1Monitor(IReloadablePtr reloadable);
        ~SigUSR1Monitor();

        int monitorFd();
        void triggered();

    private:
        Common::Threads::NotifyPipe m_pipe;
        IReloadablePtr m_reloader;
    };
}