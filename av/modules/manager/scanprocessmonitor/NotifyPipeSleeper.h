// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

// Component
#include "common/IStoppableSleeper.h"
// Product
#include "Common/Threads/NotifyPipe.h"

namespace plugin::manager::scanprocessmonitor
{
    class NotifyPipeSleeper : public common::IStoppableSleeper
    {
    public:
        explicit NotifyPipeSleeper(Common::Threads::NotifyPipe& pipe);
        bool stoppableSleep(duration_t sleepTime) override;
    private:
        Common::Threads::NotifyPipe& m_pipe;
    };
}
