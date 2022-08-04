// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

// Package
#include "IReloadable.h"
#include "SignalHandlerBase.h"
// Product
#include "Common/Threads/NotifyPipe.h"

namespace common::signals
{
    class SigUSR1Monitor : public common::signals::SignalHandlerBase
    {
    public:
        SigUSR1Monitor() = delete;
        explicit SigUSR1Monitor(IReloadablePtr reloadable);
        ~SigUSR1Monitor() override;

        void triggered();

    private:
        IReloadablePtr m_reloader;
    };
}