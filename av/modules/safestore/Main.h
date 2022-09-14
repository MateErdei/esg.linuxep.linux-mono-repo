// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

namespace safestore
{
    class Main
    {
    public:
        static int run();

    private:
        void innerRun(
            std::shared_ptr<common::signals::SigIntMonitor>& sigIntMonitor,
            std::shared_ptr<common::signals::SigTermMonitor>& sigTermMonitor);
    };
} // namespace safestore
