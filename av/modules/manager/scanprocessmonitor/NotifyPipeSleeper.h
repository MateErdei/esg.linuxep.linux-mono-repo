// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

// Component
#include "common/StoppableSleeper.h"
// Product
#include "Common/Threads/NotifyPipe.h"

namespace plugin::manager::scanprocessmonitor
{
    class NotifyPipeSleeper : public common::StoppableSleeper
    {
    public:
        explicit NotifyPipeSleeper(Common::Threads::NotifyPipe& pipe);
        bool stoppableSleep(duration_t sleepTime) override;
    private:
        Common::Threads::NotifyPipe& m_pipe;
    };
}
