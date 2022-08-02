// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "signals/LatchingSignalHandler.h"

#include <memory>

namespace common
{
    class SigIntMonitor : public common::signals::LatchingSignalHandler
    {
    public:
        explicit SigIntMonitor();
        ~SigIntMonitor() override;

        static std::shared_ptr<SigIntMonitor> getSigIntMonitor();
        bool triggered();

    };
}