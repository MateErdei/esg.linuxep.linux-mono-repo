/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReloadable.h"

#include "common/signals/SignalHandlerBase.h"

#include <Common/Threads/NotifyPipe.h>

namespace sspl::sophosthreatdetectorimpl
{
    class SigUSR1Monitor : public common::signals::SignalHandlerBase
    {
    public:
        SigUSR1Monitor() = delete;
        SigUSR1Monitor(const SigUSR1Monitor&) = delete;
        explicit SigUSR1Monitor(IReloadablePtr reloadable);
        ~SigUSR1Monitor();

        void triggered();

    private:
        IReloadablePtr m_reloader;
    };
}