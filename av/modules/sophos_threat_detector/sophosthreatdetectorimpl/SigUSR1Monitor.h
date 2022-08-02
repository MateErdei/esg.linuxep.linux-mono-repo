/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReloadable.h"

#include "common/signals/IMonitorable.h"

#include <Common/Threads/NotifyPipe.h>

namespace sspl::sophosthreatdetectorimpl
{
    class SigUSR1Monitor : public common::signals::IMonitorable
    {
    public:
        SigUSR1Monitor() = delete;
        SigUSR1Monitor(const SigUSR1Monitor&) = delete;
        explicit SigUSR1Monitor(IReloadablePtr reloadable);
        ~SigUSR1Monitor() override;

        int monitorFd() override;
        void triggered() override;

    private:
        Common::Threads::NotifyPipe m_pipe;
        IReloadablePtr m_reloader;
    };
}